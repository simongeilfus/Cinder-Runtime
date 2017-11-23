/*
 Copyright (c) 2017, Simon Geilfus
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

#include "runtime/Factory.h"
#include "cinder/Log.h"

using namespace std;
using namespace ci;

namespace runtime {

Factory& Factory::instance()
{
	static Factory factory;
	return factory;
}

namespace {

	static std::string stripNamespace( const std::string &className )
	{
		auto pos = className.find_last_of( "::" );
		if( pos == std::string::npos )
			return className;

		std::string result = className.substr( pos + 1, className.size() - pos - 1 );
		return result;
	}

} // anonymous namespace

void* Factory::allocate( size_t size, const std::type_index &typeIndex )
{
	void* instance;
	if( ! mTypes.count( typeIndex ) ) {
		instance = ::operator new( size );
	}
	else if( auto newOperator = static_cast<void*(__cdecl*)(const std::string &)>( mTypes[typeIndex].getModule()->getSymbolAddress( "rt_new_operator" ) ) ) {
		instance = newOperator( mTypes[typeIndex].getName() );
	}

	return instance;
}

void Factory::watchImpl( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<fs::path> &filePaths, const fs::path &dllPath, rt::BuildSettings settings )
{
	if( ! mTypes[typeIndex].getInstances().size() ) {
		
		if( settings.getModuleName().empty() ) {
			settings.moduleName( stripNamespace( name ) );
		}
		if( settings.getTypeName().empty() ) {
			settings.typeName( name );
		}
		
		// add class factory code generation as a prebuild step
		auto codeGenOptions = rt::CodeGeneration::Options().newOperator( name ).placementNewOperator( name );
		for( const auto &path : filePaths ) {
			if( path.extension() == ".h" || path.extension() == ".hpp" ) {
				codeGenOptions.include( path.filename().string() ); // TODO: better handling of include path (ex. #include "folder/file.h" would not work)
			}
		}
		settings.preBuildStep( make_shared<rt::CodeGeneration>( codeGenOptions ) );

		if( settings.isVerboseEnabled() ) {
			Compiler::instance().debugLog( &settings );
		}

		// and start watching the source files
		FileWatcher::instance().watch( filePaths, FileWatcher::Options().callOnWatch( false ), bind( &Factory::sourceChanged, this, placeholders::_1, typeIndex, filePaths, settings ) );
	}

	mTypes[typeIndex].getInstances().push_back( address );
}

void Factory::unwatch( const std::type_index &typeIndex, void* address )
{
	if( mTypes.count( typeIndex ) ) {
		auto &instances = mTypes[typeIndex].getInstances();
		instances.erase( std::remove( instances.begin(), instances.end(), address ), instances.end() );
	}
}

void Factory::sourceChanged( const WatchEvent &event, const std::type_index &typeIndex, const std::vector<fs::path> &filePaths, const rt::BuildSettings &settings )
{
	// unlock the dll-handle before building
	const auto &module = mTypes[typeIndex].getModule();
	module->unlockHandle();
				
	// force precompiled-header re-generation on header change
	rt::BuildSettings buildSettings = settings;
	if( event.getFile().extension() == ".h" ) {
		buildSettings.createPrecompiledHeader();
	}
	
	// initiate the build
	rt::CompilerMsvc::instance().build( filePaths.front(), buildSettings, 
		bind( &Factory::handleBuild, this, placeholders::_1, typeIndex, ( event.getFile().extension() == ".cpp" ? rt::CompilerMsvc::instance().getSymbolForVTable( buildSettings.getTypeName() ) : "" ) ) );
}

void Factory::handleBuild( const rt::BuildOutput &output, const std::type_index &typeIndex, const std::string &vtableSym )
{
	// if a new dll exists update the handle
	const auto &type = mTypes[typeIndex];
	if( fs::exists( type.getModule()->getPath() ) ) {

		// call cleanup / pre-build callbacks
		type.getModule()->getCleanupSignal().emit( *type.getModule() );
		const auto &instances = type.getInstances();
		for( size_t i = 0; i < instances.size(); ++i ) {
			if( type.getPreBuild() ) {
				type.getPreBuild()( instances[i] );
			}
		}

		// swap module's dll
		type.getModule()->updateHandle();

		// update the instances or swap vtables depending on which file has been modified
		if( ! vtableSym.empty() ) {
			swapInstancesVtables( typeIndex, vtableSym );
		}
		else {
			reconstructInstances( typeIndex );
		}
						
		type.getModule()->getChangedSignal().emit( *type.getModule() );
	}
	else {
		throw FactoryException( "Module " + output.getBuildSettings().getModuleName() + " not found at " + type.getModule()->getPath().string() );
	}
}

void Factory::swapInstancesVtables( const std::type_index &typeIndex, const std::string &vtableSym )
{
	const auto &type = mTypes[typeIndex];
	const auto &module = type.getModule();
	const auto &instances = type.getInstances();

	// Find the address of the vtable
	if( void* vtableAddress = module->getSymbolAddress( vtableSym ) ) {
		for( size_t i = 0; i < instances.size(); ++i ) {
		#if defined( CEREAL_CEREAL_HPP_ )
			std::stringstream archiveStream;
			cereal::BinaryOutputArchive outputArchive( archiveStream );
			serialize( outputArchive, instances[i] );
		#endif
			*(void **) instances[i] = vtableAddress;
									
			if( type.getPostBuild() ) {
				type.getPostBuild()( instances[i] );
			}
		#if defined( CEREAL_CEREAL_HPP_ )
			cereal::BinaryInputArchive inputArchive( archiveStream );
			serialize( inputArchive, instances[i] );
		#endif
		}
	}
}
void Factory::reconstructInstances( const std::type_index &typeIndex )
{
	const auto &type = mTypes[typeIndex];
	const auto &module = type.getModule();
	const auto &instances = type.getInstances();

	if( auto placementNewOperator = static_cast<void*(__cdecl*)(const std::string&,void*)>( module->getSymbolAddress( "rt_placement_new_operator" ) ) ) {
		// use placement new to construct new instances at the current instances addresses
		for( size_t i = 0; i < instances.size(); ++i ) {
		#if defined( CEREAL_CEREAL_HPP_ )
			std::stringstream archiveStream;
			cereal::BinaryOutputArchive outputArchive( archiveStream );
			serialize( outputArchive, instances[i] );
		#endif
			if( type.getDestructor() ) {
				type.getDestructor()( instances[i] );
			}
			placementNewOperator( type.getName(), instances[i] );
			if( type.getPostBuild() ) {
				type.getPostBuild()( instances[i] );
			}
		#if defined( CEREAL_CEREAL_HPP_ )
			cereal::BinaryInputArchive inputArchive( archiveStream );
			serialize( inputArchive, instances[i] );
		#endif
		}
	}
}

std::string Factory::stripNamespace( const std::string &className )
{
	auto pos = className.find_last_of( "::" );
	if( pos == std::string::npos )
		return className;

	std::string result = className.substr( pos + 1, className.size() - pos - 1 );
	return result;
}

ci::fs::path Factory::makeDllPath( const ci::fs::path &intermediatePath, const char *className )
{
	auto strippedClassName = stripNamespace( className );
	return intermediatePath / "runtime" / strippedClassName / "build" / ( strippedClassName + ".dll" );
}

} // namespace runtime