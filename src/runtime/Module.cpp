#include "runtime/Module.h"
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
		catch( ... ) {
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
			catch(...) {}
		}

		// rename the soon to be old module to a temp name
		mTempPath = mPath.parent_path() / ( mPath.stem().string() + "_old" + mPath.extension().string() );
		try {
			fs::rename( mPath, mTempPath );
		}
		catch(...) {}
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

		// parse the header file
		if( ! hPath.empty() ) {
			std::ifstream inputFile( hPath );
			std::ofstream outputFile( tempFolder / ( className + ".h" ) );
		
			bool reachedPragmaOnce = false;
			bool addedInclude = false;
			for( string line; std::getline( inputFile, line ); ) {
			
				// add export preprocessor
				auto classPos = line.find( "class" );
				if( classPos != string::npos ) {
					//line.replace( classPos, 5, "class __declspec(dllexport)" );
				}
				outputFile << line << endl;
			}

			// add the factory function
			outputFile << endl;
			outputFile << "#include <memory>" << endl << endl;
			/*outputFile << "extern \"C\" __declspec(dllexport) void runtimeCreateFactory( std::shared_ptr<" << className << ">* ptr )" << endl;
			outputFile << "{" << endl;
			outputFile << "\t*ptr = std::make_shared<" << className << ">();" << endl;
			outputFile << "}" << endl;*/
			outputFile << "std::shared_ptr<" << className << "> __declspec(dllexport) runtimeCreateFactory()" << endl;
			outputFile << "{" << endl;
			outputFile << "\treturn std::make_shared<" << className << ">();" << endl;
			outputFile << "}" << endl;
			outputFile << endl;
		}

		// parse the source
		bool createPch = ! fs::exists( tempFolder / "build" / ( className + "PCH.pch" ) );
		if( ! cppPath.empty() ) {
			std::ifstream inputFile( cppPath );
			std::ofstream sourceFile( tempFolder / ( className + ".cpp" ) );
			std::stringstream pchHeaderStream;
			std::stringstream pchSourceStream;
			std::vector<string> pchIncludes;
		
			// prepare pch files
			pchHeaderStream << "#pragma once" << endl << endl;
			pchSourceStream << "#include \"" << ( className + "PCH.h" ) << "\"" << endl;
			sourceFile << "#include \"" << ( className + "PCH.h" ) << "\"" << endl;
		
			// parse the source file
			int skippedIncludes = 0;
			for( string line; std::getline( inputFile, line ); ) {
				// if line is an include and is not including the main file move it to the pch header
				if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
					pchHeaderStream << line << endl;
					pchIncludes.push_back( line );
					// try to keep same number of lines
					if( skippedIncludes > 0 ) {
						sourceFile << endl;
					}
					skippedIncludes++;
				}
				// otherwise keep the include
				else if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) != string::npos ) {
					sourceFile << line << endl;
				}
				else {
					sourceFile << line << endl;
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
		mCompiler->build( tempFolder / ( className + ".cpp" ), 
						  Compiler::Options()
						  .include( tempFolder )
						  .include( path.parent_path() )
						  .preprocessorDef( "RT_MODULE_EXPORTS" )
						  .precompiledHeader( tempFolder / ( className + "PCH.cpp" ), createPch )
						  .outputPath( tempFolder / "build" )
						  .verbose( false ), 
						  [module]( const CompilationResult &results ) {

			if( ! results.hasErrors() ) {
				if( auto moduleShared = module.lock() ) {
					moduleShared->setSymbols( results.getSymbols() );
					moduleShared->getCleanupSignal().emit( moduleShared );
					moduleShared->updateHandle();
					moduleShared->getChangedSignal().emit( moduleShared );
				}
			}
		} );	
	};

	// create the empty module and add it to the list of modules
	ModuleRef module = Module::create( tempFolder / "build" / ( className + ".dll" ) );
	mModules.push_back( module );

	// watch files for changes
	auto watchPaths = [=]( const ci::fs::path &path ) {
		static int skip = 2;
		if( ! skip ) {
			buildModule( module );	
		}
		else {
			skip--;
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