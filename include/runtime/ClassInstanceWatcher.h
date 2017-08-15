#pragma once

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )

#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"

#include "runtime/Module.h"
#include "runtime/CompilerMsvc.h"

namespace runtime {

template<class T>
void watchClassInstance( T* ptr, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath )
{
	static rt::ModulePtr sModule;
	static std::vector<T*> sInstances;
	sInstances.push_back( static_cast<T*>( ptr ) );

	if( ! sModule ) {
		sModule = std::make_unique<rt::Module>( dllPath );
		ci::fs::path source = filePaths.front();
		ci::FileWatcher::instance().watch( filePaths, 
			ci::FileWatcher::Options().callOnWatch( false ),
			[&,source]( const ci::WatchEvent &event ) {  
				
				// unlock the dll-handle before building
				sModule->unlockHandle();
	
				// prepare build settings
				auto settings = rt::Compiler::BuildSettings()
					.default() // default cinder project settings
					.define( "RT_COMPILED" )
					.include( "../../../include" ); // cinder-runtime include folder
		
				// initiate the build
				rt::CompilerMsvc::instance().build( source, settings, [&]( const rt::CompilationResult &result ) {
					// if a new dll exists update the handle
					if( ci::fs::exists( sModule->getPath() ) ) {
						sModule->updateHandle();
						// and try to get a ptr to its make_unique factory function
						if( auto updatePtr = sModule->getMakeRawFactory<T>() ) {
							for( size_t i = 0; i < sInstances.size(); ++i ) {
								T* newPtr = updatePtr();
								size_t size = sizeof T;
								void* temp = malloc( size );
								memcpy( temp, newPtr, size );
								memcpy( newPtr, sInstances[i], size );
								memcpy( sInstances[i], temp, size );

								delete newPtr;
							}
						}
			
					}
				} );
			} 
		);
	}
}

} // namespace runtime

namespace rt = runtime;

#endif