#pragma once

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )

#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"

#include "runtime/Module.h"
#include "runtime/CompilerMsvc.h"

namespace runtime {

template<class T>
void watchClassInstance( T* ptr, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings(), bool unwatch = false )
{
	static rt::ModulePtr sModule;
	static std::vector<T*> sInstances;

	if( unwatch ) {
		sInstances.erase( std::remove( sInstances.begin(), sInstances.end(), ptr ), sInstances.end() );
		return;
	}
	else {
		sInstances.push_back( static_cast<T*>( ptr ) );
	}

	if( ! sModule ) {
		sModule = std::make_unique<rt::Module>( dllPath );
		ci::fs::path source = filePaths.front();
		ci::FileWatcher::instance().watch( filePaths, 
			ci::FileWatcher::Options().callOnWatch( false ),
			[&,source,settings]( const ci::WatchEvent &event ) {  
				// unlock the dll-handle before building
				sModule->unlockHandle();
		
				// initiate the build
				rt::CompilerMsvc::instance().build( source, settings, [&,event]( const rt::CompilationResult &result ) {
					// if a new dll exists update the handle
					if( ci::fs::exists( sModule->getPath() ) ) {
						sModule->updateHandle();
						// and try to get a ptr to its make_unique factory function
						if( auto makeRaw = sModule->getMakeRawFactory<T>() ) {
#if 1
							if( event.getFile().extension() == ".cpp" ) {
								// create a single new instance an use its vtable to override the originals's vtables
								T* newPtr = makeRaw();
								for( size_t i = 0; i < sInstances.size(); ++i ) {
									*(void **)sInstances[i] = *(void**) newPtr;
								}
								::operator delete( newPtr );
							}
							else if( event.getFile().extension() == ".h" ){
								// create new instances and swap with the originals
								for( size_t i = 0; i < sInstances.size(); ++i ) {
									T* newPtr = makeRaw();
									size_t size = sizeof T;
									void* temp = malloc( size );
									memcpy( temp, newPtr, size );
									memcpy( newPtr, sInstances[i], size );
									memcpy( sInstances[i], temp, size );
								
									::operator delete( newPtr );
								}
							}
#elif 0
							// create a single new instance an use its vtable to override the originals's vtables
							T* newPtr = makeRaw();
							ci::app::console() << "vtable size " << sizeof( *(void **) newPtr ) << std::endl;
							ci::app::console() << "object size " << sizeof( * newPtr ) << std::endl;
							for( size_t i = 0; i < sInstances.size(); ++i ) {
								*(void **)sInstances[i] = *(void**) newPtr;
								//std::swap( *(void **)sInstances[i], *(void**) newPtr );
							}
							::operator delete( newPtr );
#else
							//ci::app::console() << sizeof(T) << " vs " << sModule->getSizeOf()() << std::endl;
							// create new instances and swap with the originals
							for( size_t i = 0; i < sInstances.size(); ++i ) {
							#if 1
								T* newPtr = makeRaw();
								T* oldPtr = sInstances[i];
								std::memmove( oldPtr, newPtr, sizeof( T ) );
							#else
								size_t size = sizeof T;
								void* temp = malloc( size );
								memcpy( temp, newPtr, size );
								memcpy( newPtr, sInstances[i], size );
								memcpy( sInstances[i], temp, size );
							#endif								
								::operator delete( newPtr );
							}
#endif
						}
			
					}
				} );
			} 
		);
	}
}

} // namespace runtime

namespace rt = runtime;

#include "cinder/app/App.h"

#define RT_WATCH_CLASS_HEADER \
public: \
	void* operator new( size_t size ); \
	void operator delete( void* ptr ); \
private: \
	static const ci::fs::path getHeaderPath() { return ci::fs::absolute( ci::fs::path( __FILE__ ) ); } \
public:

#define __RT_WATCH_CLASS_IMPL0( Class ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	auto cppPath = ci::fs::absolute( ci::fs::path( __FILE__ ) ); \
	rt::watchClassInstance( static_cast<Class*>( ptr ), { cppPath, getHeaderPath() }, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / ( std::string( #Class ) + ".dll" ), rt::Compiler::BuildSettings().default().define( "RT_COMPILED" ).include( "../../../include" ) );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::watchClassInstance( static_cast<Class*>( ptr ), {}, "", rt::Compiler::BuildSettings(), true ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_CLASS_IMPL1( Class, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	auto cppPath = ci::fs::absolute( ci::fs::path( __FILE__ ) ); \
	rt::watchClassInstance( static_cast<Class*>( ptr ), { cppPath, getHeaderPath() }, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / ( std::string( #Class ) + ".dll" ), Settings );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::watchClassInstance( static_cast<Class*>( ptr ), {}, "", rt::Compiler::BuildSettings(), true ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_CLASS_IMPL2( Class, Header, Source, Dll, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	rt::watchClassInstance( static_cast<Class*>( ptr ), { Source, Header }, Dll, Settings );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::watchClassInstance( static_cast<Class*>( ptr ), {}, "", rt::Compiler::BuildSettings(), true ); \
	::operator delete( ptr ); \
} \

#define RT_WATCH_CLASS_INLINE( Class ) \
void* operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	auto cppPath = ci::fs::absolute( ci::fs::path( __FILE__ ) ); \
	rt::watchClassInstance( static_cast<Class*>( ptr ), { cppPath }, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / ( std::string( #Class ) + ".dll" ), rt::Compiler::BuildSettings().default().define( "RT_COMPILED" ).include( "../../../include" ) );\
	return ptr; \
} \
void operator delete( void* ptr ) \
{ \
	rt::watchClassInstance( static_cast<Class*>( ptr ), {}, "", rt::Compiler::BuildSettings(), true ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_CLASS_IMPL_SWITCH(_1,_2,_3,NAME,...) NAME
#define RT_WATCH_CLASS_IMPL( ... ) __RT_WATCH_CLASS_IMPL_SWITCH(__VA_ARGS__,__RT_WATCH_CLASS_IMPL2,__RT_WATCH_CLASS_IMPL1,__RT_WATCH_CLASS_IMPL0)(__VA_ARGS__)

#else

#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"
#define RT_WATCH_CLASS_HEADER \
public: \
	static const ci::fs::path getHeaderPath() { return ci::fs::absolute( ci::fs::path( __FILE__ ) ); }
#define RT_WATCH_CLASS_IMPL( ... )
#define RT_WATCH_CLASS_INLINE( Class )

#endif