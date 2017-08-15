#pragma once

#include <string>
#include "runtime/Module.h"
#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"

#include "runtime/CompilerMsvc.h"

namespace runtime {

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )
template<typename T>
class Swappable {
public:
	void* operator new( size_t size )
	{
		void * ptr = ::operator new( size );

		static rt::ModulePtr sModule;
		static std::vector<T*> sInstances;
		sInstances.push_back( static_cast<Test2*>( ptr ) );

		if( ! sModule ) {
			sModule = std::make_unique<rt::Module>( CI_RT_INTERMEDIATE_DIR / "runtime/Test2/Test2.dll" );
			ci::FileWatcher::instance().watch( 
				{ CI_RT_PROJECT_ROOT / "src/Test2.h", CI_RT_PROJECT_ROOT / "src/Test2.cpp" }, 
				ci::FileWatcher::Options().callOnWatch( false ),
				[]( const ci::WatchEvent &event ) {  
				
					// unlock the dll-handle before building
					sModule->unlockHandle();
	
					// prepare build settings
					auto settings = rt::Compiler::BuildSettings()
						.default() // default cinder project settings
						.define( "RT_COMPILED" )
						.include( "../../../include" ); // cinder-runtime include folder
		
					// initiate the build
					rt::CompilerMsvc::instance().build( CI_RT_PROJECT_ROOT / "src/Test2.cpp", settings, [=]( const rt::CompilationResult &result ) {
						// if a new dll exists update the handle
						if( ci::fs::exists( sModule->getPath() ) ) {
							sModule->updateHandle();
							// and try to get a ptr to its make_unique factory function
							if( auto updatePtr = sModule->getMakeRawFactory<Test2>() ) {
								for( size_t i = 0; i < sInstances.size(); ++i ) {
									Test2* newPtr = updatePtr();
									size_t size = sizeof(Test2);
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

		return ptr;
	}
	
	void operator delete( void* ptr )
	{
		::operator delete( ptr );
	}
};

#else
template<typename T>
class Swappable {
};
#endif

} // namespace runtime

class Test2 : public rt::Swappable<Test2> {
public:
	Test2();
	virtual std::string getString();
};