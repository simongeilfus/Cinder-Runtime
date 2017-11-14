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

void ClassWatcher::watchImpl( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, rt::BuildSettings settings )
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

		mModules[typeIndex] = std::make_unique<rt::Module>( dllPath );
		ci::app::console() << "Module " << dllPath << std::endl << "Source: " << filePaths.front() << std::endl;
		ci::fs::path source = filePaths.front();
		ci::FileWatcher::instance().watch( filePaths, 
			ci::FileWatcher::Options().callOnWatch( false ),
			[this,typeIndex,source,settings,name]( const ci::WatchEvent &event ) {  
				// unlock the dll-handle before building
				mModules[typeIndex]->unlockHandle();
				
				// force precompiled-header re-generation on header change
				rt::BuildSettings buildSettings = settings;
				if( event.getFile().extension() == ".h" ) {
					buildSettings.createPrecompiledHeader();
				}

				auto vtableSym = rt::CompilerMsvc::instance().getSymbolForVTable( buildSettings.getTypeName() );

				// initiate the build
				rt::CompilerMsvc::instance().build( source, buildSettings, [&,event,buildSettings, vtableSym]( const rt::CompilationResult &result ) {
					// if a new dll exists update the handle
					if( ci::fs::exists( mModules[typeIndex]->getPath() ) ) {
						const auto &callbacks = mCallbacks[typeIndex];
						mModules[typeIndex]->getCleanupSignal().emit( *mModules[typeIndex] );
						for( size_t i = 0; i < mInstances[typeIndex].size(); ++i ) {
							if( callbacks.getPreBuild() ) {
								callbacks.getPreBuild()( mInstances[typeIndex][i] );
							}
							/*if( mSetupFns[typeIndex] ) {
								mSetupFns[typeIndex]( mInstances[typeIndex][i] );
							}*/
							//callPreBuildMethod( mInstances[typeIndex][i] );
						}
						mModules[typeIndex]->updateHandle();

						if( event.getFile().extension() == ".cpp" ) {
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
						else if( event.getFile().extension() == ".h" ){
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
						
						mModules[typeIndex]->getChangedSignal().emit( *mModules[typeIndex] );
					}
					else {
						throw ClassWatcherException( "Module " + buildSettings.getModuleName() + " not found at " + mModules[typeIndex]->getPath().string() );
					}
				} );
			} 
		);
	}
}

void ClassWatcher::unwatch( const std::type_index &typeIndex, void* address )
{
	if( mInstances.count( typeIndex ) ) {
		mInstances[typeIndex].erase( std::remove( mInstances[typeIndex].begin(), mInstances[typeIndex].end(), address ), mInstances[typeIndex].end() );
	}
}

} // namespace runtime