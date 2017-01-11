/*
 Copyright (c) 2016, Simon Geilfus
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "runtime/Module.h"
#include "cinder/Log.h"
#include "Watchdog.h"
#include <iostream>
#include <fstream>
#include <sstream>

#if ! defined( WIN32_LEAN_AND_MEAN )
	#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

using namespace std;
using namespace ci;
using namespace ci::app;

namespace runtime {

ModuleRef Module::create( const ci::fs::path &path )
{
	return std::make_shared<Module>( path );
}

Module::Module( const ci::fs::path &path )
: mPath( path )
#if defined( CINDER_MSW )
, mHandle( nullptr )
#endif
{
	if( fs::exists( path ) ) {
#if defined( CINDER_MSW )
		mHandle = LoadLibrary( mPath.wstring().c_str() );
#else
#endif
	}
}

Module::~Module()
{
	// release the library
	if( mHandle ) {
#if defined( CINDER_MSW )
		FreeLibrary( static_cast<HINSTANCE>( mHandle ) );
#endif
	}

	// if the module has been updated there's propbably a temp
	// file that needs to be removed
	if( fs::exists( mTempPath ) ) {
		try {
			fs::remove( mTempPath );
		}
		catch( const fs::filesystem_error & ) {
		}
	}
}

void Module::updateHandle()
{
	if( fs::exists( mPath ) ) {
		if( mHandle != nullptr ) {
			FreeLibrary( static_cast<HINSTANCE>( mHandle ) );
		}
#if defined( CINDER_MSW )
		mHandle = LoadLibrary( mPath.wstring().c_str() );
#endif
	}
}

void Module::unlockHandle()
{
	if( fs::exists( mPath ) ) {
		// if the old temp file is still there delete it
		if( fs::exists( mTempPath ) ) {
			try {
				fs::remove( mTempPath );
			}
			catch( const fs::filesystem_error & ) {}
		}

		// rename the soon to be old module to a temp name
		mTempPath = mPath.parent_path() / ( mPath.stem().string() + "_old" + mPath.extension().string() );
		try {
			fs::rename( mPath, mTempPath );
		}
		catch( const fs::filesystem_error & ) {}
	}
}

Module::Handle Module::getHandle() const
{
	return mHandle;
}

ci::fs::path Module::getPath() const
{
	return mPath;
}

ci::fs::path Module::getTempPath() const
{
	return mTempPath;
}

bool Module::isValid() const
{
	return mHandle != nullptr;
}

void Module::setSymbols( const std::map<std::string, std::string> &symbols )
{
	mSymbols = symbols;
}

const std::map<std::string, std::string>& Module::getSymbols() const
{
	return mSymbols;
}

ci::signals::Signal<void( const ModuleRef& )>& Module::getCleanupSignal()
{
	return mCleanupSignal;
}

ci::signals::Signal<void( const ModuleRef& )>& Module::getChangedSignal()
{
	return mChangedSignal;
}

ModuleManagerRef ModuleManager::create()
{
	return std::make_shared<ModuleManager>();
}

ModuleManagerRef ModuleManager::get()
{
	static ModuleManagerRef sModuleManager = ModuleManager::create();
	return sModuleManager;
}

ModuleManager::ModuleManager()
: mCompiler( Compiler::create() )
{
}

ModuleManager::~ModuleManager()
{
	mModules.clear();
}

#define ENABLE_TIMING
#if defined( ENABLE_TIMING )
Timer timer;
#endif

ModuleRef ModuleManager::add( const ci::fs::path &path )
{
	// make sure temp folders exist
	// create parent "RTTemp" folder
	auto tempFolder = ci::app::getAppPath() / "RTTemp" / path.stem();
	if( ! ci::fs::exists( tempFolder.parent_path() ) ) {
		ci::fs::create_directory( tempFolder.parent_path() );
	}
	// create class folder
	if( ! ci::fs::exists( tempFolder ) ) {
		ci::fs::create_directory( tempFolder );
	}
	// create class build folder
	if( ! ci::fs::exists( tempFolder / "build" ) ) {
		ci::fs::create_directory( tempFolder / "build" );
	}
	else {
		// cleanup pdb files in the build folder 
		if( ci::fs::exists( tempFolder / "build" ) ) {
			ci::fs::directory_iterator end;
			for( ci::fs::directory_iterator it( tempFolder / "build" ); it != end; ++it ) {
				if( it->path().extension() == ".pdb" ) {
					try {
						ci::fs::remove( it->path() );
					} catch( const fs::filesystem_error & ){}
				}
			}
		}
	}
	
	// find out if it's a cpp/h pair or just a header
	auto hPath = ci::fs::canonical( path );
	auto cppPath = ci::fs::canonical( path );
	auto className = hPath.stem().string();
	if( cppPath.extension() == ".cpp" ) {
		if( ci::fs::exists( path.parent_path() / ( className + ".h" ) ) ) {
			hPath = path.parent_path() / ( className + ".h" );
		}
	}
	else if( cppPath.extension() == ".h" ) {
		cppPath.clear();
	}

	// build module
	auto buildModule = [=]( const std::weak_ptr<Module> &module ) {
#if defined( ENABLE_TIMING )
		timer.start();
#endif

		// create the factory source
		if( ! fs::exists( tempFolder / ( className + "Factory.cpp" ) ) ) {
			// Write the object factory file with an extern "C" function that allow instancing of the class
			std::ofstream outputFile( tempFolder / ( className + "Factory.cpp" ) );		
			outputFile << "#include <memory>" << endl << endl;
			outputFile << "#include \"" << hPath.stem().string() << ".h\"" << endl << endl;
			outputFile << "extern \"C\" __declspec(dllexport) void __cdecl runtimeCreateFactory( std::shared_ptr<" << className << ">* ptr )" << endl;
			outputFile << "{" << endl;
			outputFile << "\t*ptr = std::make_shared<" << className << ">();" << endl;
			outputFile << "}" << endl;
			outputFile << endl;
		}
		
		// parse the source for headers
		bool createPch = ! fs::exists( tempFolder / "build" / ( className + "PCH.pch" ) );
		if( ! cppPath.empty() ) {

			std::ifstream inputFile( cppPath );
			std::stringstream pchHeaderStream;
			std::stringstream pchSourceStream;
			std::vector<string> pchIncludes;
		
			// prepare pch files
			pchHeaderStream << "#pragma once" << endl << endl;
			pchSourceStream << "#include \"" << ( className + "PCH.h" ) << "\"" << endl;
			
			// parse the source file
			int skippedIncludes = 0;
			for( string line; std::getline( inputFile, line ); ) {
				// if line is an include and is not including the main file move it to the pch header
				if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
					pchHeaderStream << line << endl;
					pchIncludes.push_back( line );
				}
				// #pragma hdrstop support
				else if( line.find( "#pragma") != string::npos && line.find( "hdrstop" ) != string::npos ) {
					break;
				}
			}

			// check if the pch header needs to be written for the first time or updated
			bool writePchHeader = false;
			bool pchHeaderExists = fs::exists( tempFolder / ( className + "PCH.h" ) );
			if( pchHeaderExists ) {
				std::vector<string> pchHeaderFileIncludes;
				std::ifstream pchHeaderFile( tempFolder / ( className + "PCH.h" ) );
				for( string line; std::getline( pchHeaderFile, line ); ) {
					if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
						pchHeaderFileIncludes.push_back( line );
					}				
				}
				writePchHeader = ( pchIncludes != pchHeaderFileIncludes );
			}
			if( writePchHeader || ! pchHeaderExists ) {
				std::ofstream pchHeaderFile( tempFolder / ( className + "PCH.h" ) );
				pchHeaderFile << pchHeaderStream.str();
				createPch = true;
			}

			// check if the pch source file needs to be written for the first time
			if( ! fs::exists( tempFolder / ( className + "PCH.cpp" ) ) ) {
				std::ofstream pchSourceFile( tempFolder / ( className + "PCH.cpp" ) );
				pchSourceFile << pchSourceStream.str();			
				createPch = true;
			}
		}

		// compile module
		if( auto moduleShared = module.lock() ) {
			moduleShared->unlockHandle();
		}

		// prepare build options
		auto buildOptions = Compiler::Options()
			.include( tempFolder )
			.include( path.parent_path() )
			.additionalCompileList( { tempFolder / ( className + "Factory.cpp" ) } )
			.outputPath( tempFolder / "build" )
			//.linkAppObjs( false )
			.verbose( false );

		// enables precompiled header only on .cpp files for the moment
		if( ! cppPath.empty() ) {
			buildOptions.precompiledHeader( tempFolder / ( className + "PCH.cpp" ), createPch );
			buildOptions.forceInclude( className + "PCH.h" );
		}

		// start the building process
		mCompiler->build( ! cppPath.empty() ? cppPath : hPath, buildOptions, [module,className]( const CompilationResult &results ) {

			if( ! results.hasErrors() ) {
				if( auto moduleShared = module.lock() ) {
					moduleShared->setSymbols( results.getSymbols() );
					moduleShared->getCleanupSignal().emit( moduleShared );
					moduleShared->updateHandle();
#if defined( ENABLE_TIMING )
					timer.stop();
					CI_LOG_I( timer.getSeconds() * 1000.0 << "ms" );
#endif
					moduleShared->getChangedSignal().emit( moduleShared );
				}
			}
			else {
				CI_LOG_E( "Error recompiling " << className );
				for( auto error : results.getErrors() ) {
					std::transform( error.begin(), error.end(), error.begin(), ::tolower );
					app::console() << "1>" << error << endl;
				}
			}
		} );	
	};

	// create the empty module and add it to the list of modules
	ModuleRef module = Module::create( tempFolder / "build" / ( className + ".dll" ) );
	mModules.push_back( module );

	// watch files for changes
	auto watchPaths = [=]( const ci::fs::path &path ) {
		static map<fs::path,int> skip;
		if( ! skip.count( path ) ) skip[path] = 1;
		if( ! skip[path] ) {
			buildModule( module );	
		}
		else {
			skip[path]--;
		}
	};
	if( ! hPath.empty() ) {
		wd::watch( hPath, watchPaths );
	}
	if( ! cppPath.empty() ) {
		wd::watch( cppPath, watchPaths );
	}

	return module;
}

} // namespace runtime