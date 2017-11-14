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

#include "runtime/ClassWatcher.h"

using namespace std;
using namespace ci;

namespace runtime {

ClassWatcher& ClassWatcher::instance()
{
	static ClassWatcher classWatcher;
	return classWatcher;
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

void ClassWatcher::watchImpl( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<fs::path> &filePaths, const fs::path &dllPath, rt::BuildSettings settings )
{
	if( ! mInstances.count( typeIndex ) ) {
		mInstances[typeIndex].push_back( address );
	}

	if( ! mModules.count( typeIndex ) ) {
		
		if( settings.getModuleName().empty() ) {
			settings.moduleName( stripNamespace( name ) );
		}
		if( settings.getTypeName().empty() ) {
			settings.typeName( name );
		}

		if( settings.isVerboseEnabled() ) {
			Compiler::instance().debugLog( &settings );
		}

		// initialize the class module
		mModules[typeIndex] = std::make_unique<rt::Module>( dllPath );
		// and start watching the source files
		FileWatcher::instance().watch( filePaths, FileWatcher::Options().callOnWatch( false ), bind( &ClassWatcher::sourceChanged, this, placeholders::_1, typeIndex, filePaths, settings ) );
	}
}

void ClassWatcher::unwatch( const std::type_index &typeIndex, void* address )
{
	if( mInstances.count( typeIndex ) ) {
		mInstances[typeIndex].erase( std::remove( mInstances[typeIndex].begin(), mInstances[typeIndex].end(), address ), mInstances[typeIndex].end() );
	}
}

void ClassWatcher::sourceChanged( const WatchEvent &event, const std::type_index &typeIndex, const std::vector<fs::path> &filePaths, const rt::BuildSettings &settings )
{
	// unlock the dll-handle before building
	mModules[typeIndex]->unlockHandle();
				
	// force precompiled-header re-generation on header change
	rt::BuildSettings buildSettings = settings;
	if( event.getFile().extension() == ".h" ) {
		buildSettings.createPrecompiledHeader();
	}
	
	// initiate the build
	rt::CompilerMsvc::instance().build( filePaths.front(), buildSettings, 
		bind( &ClassWatcher::handleBuild, this, placeholders::_1, typeIndex, ( event.getFile().extension() == ".cpp" ? rt::CompilerMsvc::instance().getSymbolForVTable( buildSettings.getTypeName() ) : "" ) ) );
}

void ClassWatcher::handleBuild( const rt::CompilationResult &result, const std::type_index &typeIndex, const std::string &vtableSym )
{
	// if a new dll exists update the handle
	if( fs::exists( mModules[typeIndex]->getPath() ) ) {

		// call cleanup / pre-build callbacks
		const auto &callbacks = mCallbacks[typeIndex];
		mModules[typeIndex]->getCleanupSignal().emit( *mModules[typeIndex] );
		for( size_t i = 0; i < mInstances[typeIndex].size(); ++i ) {
			if( callbacks.getPreBuild() ) {
				callbacks.getPreBuild()( mInstances[typeIndex][i] );
			}
		}

		// swap module's dll
		mModules[typeIndex]->updateHandle();

		// update the instances or swap vtables depending on which file has been modified
		if( ! vtableSym.empty() ) {
			swapInstancesVtables( typeIndex, vtableSym );
		}
		else {
			reconstructInstances( typeIndex );
		}
						
		mModules[typeIndex]->getChangedSignal().emit( *mModules[typeIndex] );
	}
	else {
		//throw ClassWatcherException( "Module " + buildSettings.getModuleName() + " not found at " + mModules[typeIndex]->getPath().string() );
	}
}

void ClassWatcher::swapInstancesVtables( const std::type_index &typeIndex, const std::string &vtableSym )
{
	const auto &callbacks = mCallbacks[typeIndex];
	// Find the address of the vtable
	if( void* vtableAddress = mModules[typeIndex]->getSymbolAddress( vtableSym ) ) {
		for( size_t i = 0; i < mInstances[typeIndex].size(); ++i ) {
		#if defined( CEREAL_CEREAL_HPP_ )
			std::stringstream archiveStream;
			cereal::BinaryOutputArchive outputArchive( archiveStream );
			serialize( outputArchive, mInstances[typeIndex][i] );
		#endif
			*(void **)mInstances[typeIndex][i] = vtableAddress;
									
			if( callbacks.getPostBuild() ) {
				callbacks.getPostBuild()( mInstances[typeIndex][i] );
			}
		#if defined( CEREAL_CEREAL_HPP_ )
			cereal::BinaryInputArchive inputArchive( archiveStream );
			serialize( inputArchive, mInstances[typeIndex][i] );
		#endif
		}
	}
}
void ClassWatcher::reconstructInstances( const std::type_index &typeIndex )
{
	const auto &callbacks = mCallbacks[typeIndex];
	if( auto placementNewOperator = static_cast<void*(__cdecl*)(void*)>( mModules[typeIndex]->getSymbolAddress( "rt_placement_new_operator" ) ) ) {
		// use placement new to construct new instances at the current instances addresses
		for( size_t i = 0; i < mInstances[typeIndex].size(); ++i ) {
		#if defined( CEREAL_CEREAL_HPP_ )
			std::stringstream archiveStream;
			cereal::BinaryOutputArchive outputArchive( archiveStream );
			serialize( outputArchive, mInstances[typeIndex][i] );
		#endif
			if( callbacks.getDestructor() ) {
				callbacks.getDestructor()( mInstances[typeIndex][i] );
			}
			placementNewOperator( mInstances[typeIndex][i] );
			if( callbacks.getPostBuild() ) {
				callbacks.getPostBuild()( mInstances[typeIndex][i] );
			}
		#if defined( CEREAL_CEREAL_HPP_ )
			cereal::BinaryInputArchive inputArchive( archiveStream );
			serialize( inputArchive, mInstances[typeIndex][i] );
		#endif
		}
	}
}

} // namespace runtime