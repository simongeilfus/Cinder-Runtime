#pragma once

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )

#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"

#include "runtime/Module.h"
#include "runtime/CompilerMsvc.h"

#if defined( CEREAL_CEREAL_HPP_ )
#include <cereal/details/traits.hpp>
#include <cereal/archives/binary.hpp>
#endif

namespace runtime {

template<class T>
class ClassWatcher {
public:
	
	//! Returns the global ClassWatcher instance
	static ClassWatcher& instance();
	
	//! Adds an instance to the ClassWatcher watch list
	void watch( T* ptr, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings( true ) );
	//! Removes an instance from ClassWatcher watch list
	void unwatch( T* ptr );

	enum class Method { RECONSTRUCT, SWAP_VTABLE };

	class Options {
	public:
		Options() : mMethod( Method::SWAP_VTABLE ), mBuildSettings( true ) {}
		//! Adds an extra include folder to the compiler BuildSettings
		Options& source( const ci::fs::path &path ) { mSource = path; return *this; }
		//! Adds an extra include folder to the compiler BuildSettings
		// Options& header( const ci::fs::path &path );
		//! Adds an extra include folder to the compiler BuildSettings
		Options& className( const std::string &name ) { mClassName = name; return *this; }
		//! Adds an extra include folder to the compiler BuildSettings
		Options& method( Method method ) { mMethod = method; return *this; }
		//! Adds an extra include folder to the compiler BuildSettings
		Options& buildSettings( const CompilerMsvc::BuildSettings &buildSettings ) { mBuildSettings = buildSettings; return *this; }
		
		//! Adds an extra include folder to the compiler BuildSettings
		// Options& additionalSources( bool watch = true );
		//! Adds an extra include folder to the compiler BuildSettings
		// Options& objectFiles( bool watch = true );

	protected:
		std::string					mClassName;
		ci::fs::path				mSources;
		Method						mMethod;
		CompilerMsvc::BuildSettings mBuildSettings;
		friend class ClassWatcher<T>;
	};
	
	//! Returns the signal used to notify when the Module/Handle is about to be unloaded
	ci::signals::Signal<void(const Module&)>& getCleanupSignal() { return mModule->getCleanupSignal(); }
	//! Returns the signal used to notify Module/Handle changes
	ci::signals::Signal<void(const Module&)>& getChangedSignal() { return mModule->getChangedSignal(); }
	
	const rt::Module& getModule() const { return *mModule; }
	rt::Module& getModule() { return *mModule; }
	
	//! Adds an extra include folder to the compiler BuildSettings
	const Options&	getOptions() const { return mOptions; }
	//! Adds an extra include folder to the compiler BuildSettings
	Options&		getOptions() { return mOptions; }
	//! Adds an extra include folder to the compiler BuildSettings
	void			setOptions( const Options& options ) { mOptions = options; }
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
	
#if defined( CEREAL_CEREAL_HPP_ )
	template<typename Archive, typename U=T,std::enable_if_t<(cereal::traits::is_output_serializable<U,Archive>::value&&cereal::traits::is_input_serializable<U,Archive>::value),int> = 0> 
	void serialize( Archive &archive, U* t ) 
	{ 
		try { 
			archive( *t ); 
		}
		catch( const std::exception &exc ) {}
	}
	template<typename Archive, typename U=T,std::enable_if_t<!(cereal::traits::is_output_serializable<U,Archive>::value&&cereal::traits::is_input_serializable<U,Archive>::value),int> = 0> void serialize( Archive &archive, U* t ) {} // no-op
#endif

	Options			mOptions;
	rt::ModulePtr	mModule;
	std::vector<T*> mInstances;
};


template<class T>
typename ClassWatcher<T>& ClassWatcher<T>::instance()
{
	static std::unique_ptr<ClassWatcher<T>> watcher = std::make_unique<ClassWatcher<T>>();
	return *watcher;
}

template<class T>
void ClassWatcher<T>::watch( T* ptr, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings( true ) )
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
				if( buildSettings.getModuleName().empty() ) {
					buildSettings.moduleName( name );
				}

				// initiate the build
				rt::CompilerMsvc::instance().build( source, buildSettings, [&,event,buildSettings]( const rt::CompilationResult &result ) {
					// if a new dll exists update the handle
					if( ci::fs::exists( mModule->getPath() ) ) {
						mModule->getCleanupSignal().emit( *mModule );
						mModule->updateHandle();

						if( event.getFile().extension() == ".cpp" ) {
							// Find the address of the vtable
							if( void* vtableAddress = mModule->getSymbolAddress( "??_7" + buildSettings.getModuleName() + "@@6B@" ) ) {
								for( size_t i = 0; i < mInstances.size(); ++i ) {
									callPreRuntimeBuild( mInstances[i] );
								#if defined( CEREAL_CEREAL_HPP_ )
									std::stringstream archiveStream;
									cereal::BinaryOutputArchive outputArchive( archiveStream );
									serialize( outputArchive, mInstances[i] );
								#endif
									*(void **)mInstances[i] = vtableAddress;
									callPostRuntimeBuild( mInstances[i] );
								#if defined( CEREAL_CEREAL_HPP_ )
									cereal::BinaryInputArchive inputArchive( archiveStream );
									serialize( inputArchive, mInstances[i] );
								#endif
								}
							}
						}
						else if( event.getFile().extension() == ".h" ){
							if( auto placementNewOperator = static_cast<T*(__cdecl*)(T*)>( mModule->getSymbolAddress( "rt_placement_new_operator" ) ) ) {
								// use placement new to construct new instances at the current instances addresses
								for( size_t i = 0; i < mInstances.size(); ++i ) {
									callPreRuntimeBuild( mInstances[i] );
								#if defined( CEREAL_CEREAL_HPP_ )
									std::stringstream archiveStream;
									cereal::BinaryOutputArchive outputArchive( archiveStream );
									serialize( outputArchive, mInstances[i] );
								#endif
									mInstances[i]->~T();
									placementNewOperator( mInstances[i] );
									callPostRuntimeBuild( mInstances[i] );
								#if defined( CEREAL_CEREAL_HPP_ )
									cereal::BinaryInputArchive inputArchive( archiveStream );
									serialize( inputArchive, mInstances[i] );
								#endif
								}
							}
						}
						
						mModule->getChangedSignal().emit( *mModule );
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
	auto buildSettings = rt::Compiler::BuildSettings( true ); \
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), { cppPath, __rt_getHeaderPath() }, buildSettings.getIntermediatePath() / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ) );\
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
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), { cppPath, __rt_getHeaderPath() }, Settings.getIntermediatePath() / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ), Settings );\
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
	auto buildSettings = rt::Compiler::BuildSettings( true ); \
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), sources, buildSettings.getIntermediatePath() / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ) );\
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
	rt::ClassWatcher<Class>::instance().watch( static_cast<Class*>( ptr ), std::string( #Class ), sources, Settings.getIntermediatePath() / "runtime" / std::string( #Class ) / "build" / ( std::string( #Class ) + ".dll" ), Settings );\
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
#define RT_WATCH_INLINE( ... ) __RT_WATCH_EXPAND(__RT_WATCH_INLINE_SWITCH(__VA_ARGS__,__RT_WATCH_INLINE2,__RT_WATCH_INLINE1))__RT_WATCH_EXPAND((__VA_ARGS__))

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