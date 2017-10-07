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
	void watch( T* ptr, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings() );
	//! Removes an instance from ClassWatcher watch list
	void unwatch( T* ptr );

	enum class Method { RECONSTRUCT, SWAP_VTABLE };

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
	
	template<typename, typename C>
	struct hasPreRuntimeBuild {
		static_assert( std::integral_constant<C, false>::value, "Second template parameter needs to be of function type." );
	};

	template<typename C, typename Ret, typename... Args>
	struct hasPreRuntimeBuild<C, Ret(Args...)> {
	private:
		template<typename U> static constexpr auto check(U*) -> typename std::is_same<decltype( std::declval<U>().preRuntimeBuild( std::declval<Args>()... ) ),Ret>::type;
		template<typename> static constexpr std::false_type check(...);
		typedef decltype(check<C>(0)) type;
	public:
		static constexpr bool value = type::value;
	};

	template<typename U=T,std::enable_if_t<hasPreRuntimeBuild<U,void()>::value,int> = 0> void callPreRuntimeBuild( U* t ) { t->preRuntimeBuild(); }
	template<typename U=T,std::enable_if_t<!hasPreRuntimeBuild<U,void()>::value,int> = 0> void callPreRuntimeBuild( U* t ) {} // no-op
	
	template<typename, typename C>
	struct hasPostRuntimeBuild {
		static_assert( std::integral_constant<C, false>::value, "Second template parameter needs to be of function type." );
	};

	template<typename C, typename Ret, typename... Args>
	struct hasPostRuntimeBuild <C, Ret(Args...)> {
	private:
		template<typename U> static constexpr auto check(U*) -> typename std::is_same<decltype( std::declval<U>().postRuntimeBuild( std::declval<Args>()... ) ),Ret>::type;
		template<typename> static constexpr std::false_type check(...);
		typedef decltype(check<C>(0)) type;
	public:
		static constexpr bool value = type::value;
	};

	template<typename U=T,std::enable_if_t<hasPostRuntimeBuild<U,void()>::value,int> = 0> void callPostRuntimeBuild( U* t ) { t->postRuntimeBuild(); }
	template<typename U=T,std::enable_if_t<!hasPostRuntimeBuild<U,void()>::value,int> = 0> void callPostRuntimeBuild( U* t ) {} // no-op

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
void ClassWatcher<T>::watch( T* ptr, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings() )
{
	mInstances.push_back( static_cast<T*>( ptr ) );
	
	if( ! mModule ) {
		mModule = std::make_unique<rt::Module>( dllPath );

		ci::fs::path source = filePaths.front();
		ci::FileWatcher::instance().watch( filePaths, 
			ci::FileWatcher::Options().callOnWatch( false ),
			[&,source,settings,name]( const ci::WatchEvent &event ) {  
				// unlock the dll-handle before building
				mModule->unlockHandle();
				
				// force precompiled-header re-generation on header change
				rt::Compiler::BuildSettings buildSettings = settings;
				if( event.getFile().extension() == ".h" ) {
					buildSettings.createPrecompiledHeader();
				}

				// initiate the build
				rt::CompilerMsvc::instance().build( source, buildSettings, [&,event,name]( const rt::CompilationResult &result ) {
					// if a new dll exists update the handle
					if( ci::fs::exists( mModule->getPath() ) ) {
						mModule->updateHandle();

						if( event.getFile().extension() == ".cpp" ) {
							// Find the address of the vtable
							// https://social.msdn.microsoft.com/Forums/vstudio/en-US/0cb15e28-4852-4cba-b63d-8a0de6e88d5f/accessing-the-vftable-vfptr-without-constructing-the-object?forum=vclanguage
							// https://www.gamedev.net/forums/topic/392971-c-compile-time-retrival-of-a-classs-vtable-solved/?page=2
							// https://www.gamedev.net/forums/topic/460569-c-compile-time-retrival-of-a-classs-vtable-solution-2/
							if( void* vtableAddress = mModule->getSymbolAddress( "??_7" + name + "@@6B@" ) ) {
								for( size_t i = 0; i < mInstances.size(); ++i ) {
									callPreRuntimeBuild( mInstances[i] );
									*(void **)mInstances[i] = vtableAddress;
									callPostRuntimeBuild( mInstances[i] );
								}
							}
						}
						else if( event.getFile().extension() == ".h" ){
							if( auto placementNewOperator = static_cast<T*(__cdecl*)(T*)>( mModule->getSymbolAddress( "rt_placement_new_operator" ) ) ) {
								// use placement new to construct new instances at the current instances addresses
								for( size_t i = 0; i < mInstances.size(); ++i ) {
									callPreRuntimeBuild( mInstances[i] );
									mInstances[i]->~T();
									placementNewOperator( mInstances[i] );
									callPostRuntimeBuild( mInstances[i] );
								}
							}
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
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), { cppPath, __rt_getHeaderPath() }, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ), rt::Compiler::BuildSettings().default().define( "RT_COMPILED" ).include( "../../../include" ) );\
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
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), { cppPath, __rt_getHeaderPath() }, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ), Settings );\
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
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), { Source, Header }, Dll, Settings );\
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
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), sources, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ), rt::Compiler::BuildSettings().default().define( "RT_COMPILED" ).include( "../../../include" ) );\
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
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), sources, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ), Settings );\
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