#pragma once

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )

#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"

#include "runtime/Module.h"
#include "runtime/CompilerMsvc.h"

namespace runtime {

template<class T>
class ClassWatcher {
public:
	
	//! Returns the global ClassWatcher instance
	static ClassWatcher& instance();
	
	//! Adds an instance to the ClassWatcher watch list
	void watch( T* ptr , const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings() );
	//! Removes an instance from ClassWatcher watch list
	void unwatch( T* ptr );

	enum class Method { OVERRIDE_PTR, SWAP_VTABLE, HEADER_OVERRIDE_PTR_OR_CPP_SWAP_VTABLE };

	class Options {
	public:
		//! Adds an extra include folder to the compiler BuildSettings
		Options& source( const ci::fs::path &path );
		//! Adds an extra include folder to the compiler BuildSettings
		Options& header( const ci::fs::path &path );
		//! Adds an extra include folder to the compiler BuildSettings
		Options& className( const ci::fs::path &path );
		//! Adds an extra include folder to the compiler BuildSettings
		Options& method( Method method );
		//! Adds an extra include folder to the compiler BuildSettings
		Options& buildSettings( const CompilerMsvc::BuildSettings &buildSettings );
		
		//! Adds an extra include folder to the compiler BuildSettings
		Options& watchAdditionalSources( bool watch = true );
		//! Adds an extra include folder to the compiler BuildSettings
		Options& watchObjectFiles( bool watch = true );

	protected:
		std::string		mClassName;
		ci::fs::path	mSources;
		ci::fs::path	mDllPath;
		friend class ClassWatcher<T>;
	};
	
	//! Adds an extra include folder to the compiler BuildSettings
	const Options&	getOptions() const;
	//! Adds an extra include folder to the compiler BuildSettings
	void			setOptions( const Options& options );
	
	//! Returns the signal used to notify when the Module/Handle is about to be unloaded
	ci::signals::Signal<void(const Module&)>& getCleanupSignal();
	//! Returns the signal used to notify Module/Handle changes
	ci::signals::Signal<void(const Module&)>& getChangedSignal();

protected:
	Options			mOptions;
	rt::ModulePtr	mModule;
	std::vector<T*> mInstances;
};


template<class T>
typename ClassWatcher<T>& ClassWatcher<T>::instance()
{
	static std::unique_ptr<ClassWatcher<T>> watcher = std::make_unique<ClassWatcher<T>>();
	return *watcher.get();
}

template<class T>
void ClassWatcher<T>::watch( T* ptr , const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings() )
{
	mInstances.push_back( static_cast<T*>( ptr ) );
	
	if( ! mModule ) {
		mModule = std::make_unique<rt::Module>( dllPath );

		ci::fs::path source = filePaths.front();
		ci::FileWatcher::instance().watch( filePaths, 
			ci::FileWatcher::Options().callOnWatch( false ),
			[&,source,settings]( const ci::WatchEvent &event ) {  
				// unlock the dll-handle before building
				mModule->unlockHandle();
		
				// initiate the build
				rt::CompilerMsvc::instance().build( source, settings, [&,event]( const rt::CompilationResult &result ) {
					// if a new dll exists update the handle
					if( ci::fs::exists( mModule->getPath() ) ) {
						mModule->updateHandle();
						// and try to get a ptr to its make_unique factory function
						if( auto makeRaw = mModule->getMakeRawFactory<T>() ) {
#if 1
							if( event.getFile().extension() == ".cpp" ) {
								// create a single new instance an use its vtable to override the originals's vtables
								T* newPtr = makeRaw();
								for( size_t i = 0; i < mInstances.size(); ++i ) {
									*(void **)mInstances[i] = *(void**) newPtr;
								}
								::operator delete( newPtr );
							}
							else if( event.getFile().extension() == ".h" ){
								// create new instances and swap with the originals
								for( size_t i = 0; i < mInstances.size(); ++i ) {
									T* newPtr = makeRaw();
									size_t size = sizeof T;
									void* temp = malloc( size );
									memcpy( temp, newPtr, size );
									memcpy( newPtr, mInstances[i], size );
									memcpy( mInstances[i], temp, size );
								
									::operator delete( newPtr );
								}
							}
#elif 0
							// create a single new instance an use its vtable to override the originals's vtables
							T* newPtr = makeRaw();
							ci::app::console() << "vtable size " << sizeof( *(void **) newPtr ) << std::endl;
							ci::app::console() << "object size " << sizeof( * newPtr ) << std::endl;
							for( size_t i = 0; i < mInstances.size(); ++i ) {
								*(void **)mInstances[i] = *(void**) newPtr;
								//std::swap( *(void **)mInstances[i], *(void**) newPtr );
							}
							::operator delete( newPtr );
#else
							//ci::app::console() << sizeof(T) << " vs " << mModule->getSizeOf()() << std::endl;
							// create new instances and swap with the originals
							for( size_t i = 0; i < mInstances.size(); ++i ) {
							#if 1
								T* newPtr = makeRaw();
								T* oldPtr = mInstances[i];
								std::memmove( oldPtr, newPtr, sizeof( T ) );
							#else
								size_t size = sizeof T;
								void* temp = malloc( size );
								memcpy( temp, newPtr, size );
								memcpy( newPtr, mInstances[i], size );
								memcpy( mInstances[i], temp, size );
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

template<class T>
void ClassWatcher<T>::unwatch( T* ptr )
{
	mInstances.erase( std::remove( mInstances.begin(), mInstances.end(), ptr ), mInstances.end() );
}


template<class T>
ci::signals::Signal<void( const Module& )>& ClassWatcher<T>::getCleanupSignal()
{
	return mModule->getCleanupSignal();
}
template<class T>
ci::signals::Signal<void( const Module& )>& ClassWatcher<T>::getChangedSignal()
{
	return mModule->getChangedSignal();
}

} // namespace runtime

namespace rt = runtime;

#include "cinder/app/App.h"

#define RT_WATCH_HEADER \
public: \
	void* operator new( size_t size ); \
	void operator delete( void* ptr ); \
private: \
	static const ci::fs::path __rt_getHeaderPath() { return ci::fs::absolute( ci::fs::path( __FILE__ ) ); } \
public:

#define __RT_WATCH_IMPL1( Class ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	auto cppPath = ci::fs::absolute( ci::fs::path( __FILE__ ) ); \
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), { cppPath, __rt_getHeaderPath() }, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / ( std::string( #Class ) + ".dll" ), rt::Compiler::BuildSettings().default().define( "NO_RT_COMPILED" ).include( "../../../include" ) );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::ClassWatcher<Class>::instance().unwatch( static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_IMPL2( Class, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	auto cppPath = ci::fs::absolute( ci::fs::path( __FILE__ ) ); \
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), { cppPath, __rt_getHeaderPath() }, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / ( std::string( #Class ) + ".dll" ), Settings );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::ClassWatcher<Class>::instance().unwatch( static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_IMPL3( Class, Header, Source, Dll, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), { Source, Header }, Dll, Settings );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::ClassWatcher<Class>::instance().unwatch( static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_INLINE1( Class ) \
public: \
void* operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	auto headerPath = ci::fs::absolute( ci::fs::path( __FILE__ ) ); \
	std::vector<ci::fs::path> sources; \
	if( ci::fs::exists( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) ) ) { \
		sources.push_back( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) ); \
	} \
	sources.push_back( headerPath ); \
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), sources, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / ( std::string( #Class ) + ".dll" ), rt::Compiler::BuildSettings().default().define( "NO_RT_COMPILED" ).include( "../../../include" ) );\
	return ptr; \
} \
void operator delete( void* ptr ) \
{ \
	rt::ClassWatcher<Class>::instance().unwatch( static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_INLINE2( Class, Settings ) \
public: \
void* operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	auto headerPath = ci::fs::absolute( ci::fs::path( __FILE__ ) ); \
	std::vector<ci::fs::path> sources; \
	if( ci::fs::exists( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) ) ) { \
		sources.push_back( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) ); \
	} \
	sources.push_back( headerPath ); \
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), sources, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / ( std::string( #Class ) + ".dll" ), Settings );\
	return ptr; \
} \
void operator delete( void* ptr ) \
{ \
	rt::ClassWatcher<Class>::instance().unwatch( static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_WATCH_IMPL_SWITCH(_1,_2,_3,NAME,...) NAME
#define __RT_WATCH_INLINE_SWITCH(_1,_2,NAME,...) NAME
#define __RT_WATCH_EXPAND(x) x
#define RT_WATCH_IMPL( ... ) __RT_WATCH_EXPAND(__RT_WATCH_IMPL_SWITCH(__VA_ARGS__,__RT_WATCH_IMPL3,__RT_WATCH_IMPL2,__RT_WATCH_IMPL1))__RT_WATCH_EXPAND((__VA_ARGS__))
#define RT_WATCH_INLINE( ... ) __RT_WATCH_INLINE_SWITCH(__VA_ARGS__,__RT_WATCH_INLINE2,__RT_WATCH_INLINE1)(__VA_ARGS__)

#else

#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"
#define RT_WATCH_HEADER \
private: \
	static const ci::fs::path __rt_getHeaderPath() { return ci::fs::absolute( ci::fs::path( __FILE__ ) ); } \
public:
#define RT_WATCH_IMPL( ... )
#define RT_WATCH_INLINE( ... )

#endif