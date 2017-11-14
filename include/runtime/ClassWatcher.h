#pragma once

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )

#include <typeindex>

#include "cinder/Exception.h"
#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"

#include "runtime/Module.h"
#include "runtime/CompilerMsvc.h"

#if defined( CEREAL_CEREAL_HPP_ )
#include <cereal/details/traits.hpp>
#include <cereal/archives/binary.hpp>
#endif

#if ! defined( RT_PRE_BUILD_METHOD_NAME )
#define RT_PRE_BUILD_METHOD_NAME cleanup
#endif

#if ! defined( RT_POST_BUILD_METHOD_NAME )
#define RT_POST_BUILD_METHOD_NAME setup
#endif

namespace runtime {

class ClassWatcher {
public:
	//! Returns the global ClassWatcher instance
	static ClassWatcher& instance();
	
	//! Adds an instance to the ClassWatcher watch list
	template<typename T>
	void watch( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, rt::BuildSettings settings = rt::BuildSettings().vcxproj() );
	//! Removes an instance from ClassWatcher watch list
	void unwatch( const std::type_index &typeIndex, void* address );

protected:
	template<typename T>
	void initCallbacks( const std::type_index &typeIndex );
	void watchImpl( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, rt::BuildSettings settings = rt::BuildSettings().vcxproj() );
	
	class Callbacks {
	public:
		template<typename T>
		void init();
		
		const std::function<void(void*)>& getPreBuild() const { return mPreBuild; }
		const std::function<void(void*)>& getPostBuild() const { return mPostBuild; }
		const std::function<void(void*)>& getDestructor() const { return mDestructor; }

	protected:
		
		// prebuild detection / snifae
		template<typename, typename C>
		struct hasPreBuildMethod {
			static_assert( std::integral_constant<C, false>::value, "Second template parameter needs to be of function type." );
		};
		template<typename C, typename Ret, typename... Args>
		struct hasPreBuildMethod<C, Ret(Args...)> {
		private:
			template<typename U> static constexpr auto check(U*) -> typename std::is_same<decltype( std::declval<U>().RT_PRE_BUILD_METHOD_NAME( std::declval<Args>()... ) ),Ret>::type;
			template<typename> static constexpr std::false_type check(...);
			typedef decltype(check<C>(0)) type;
		public:
			static constexpr bool value = type::value;
		};

		template<typename U,std::enable_if_t<hasPreBuildMethod<U,void()>::value,int> = 0> void callPreBuildMethod( U* t ) { t->RT_PRE_BUILD_METHOD_NAME(); }
		template<typename U,std::enable_if_t<!hasPreBuildMethod<U,void()>::value,int> = 0> void callPreBuildMethod( U* t ) {} // no-op
		
		// postbuild detection / snifae	
		template<typename, typename C>
		struct hasPostBuildMethod {
			static_assert( std::integral_constant<C, false>::value, "Second template parameter needs to be of function type." );
		};
		template<typename C, typename Ret, typename... Args>
		struct hasPostBuildMethod <C, Ret(Args...)> {
		private:
			template<typename U> static constexpr auto check(U*) -> typename std::is_same<decltype( std::declval<U>().RT_POST_BUILD_METHOD_NAME( std::declval<Args>()... ) ),Ret>::type;
			template<typename> static constexpr std::false_type check(...);
			typedef decltype(check<C>(0)) type;
		public:
			static constexpr bool value = type::value;
		};

		template<typename U,std::enable_if_t<hasPostBuildMethod<U,void()>::value,int> = 0> void callPostBuildMethod( U* t ) { t->RT_POST_BUILD_METHOD_NAME(); }
		template<typename U,std::enable_if_t<!hasPostBuildMethod<U,void()>::value,int> = 0> void callPostBuildMethod( U* t ) {} // no-op
	
		// cereal detection / snifae
	#if defined( CEREAL_CEREAL_HPP_ )
		template<typename Archive, typename U,std::enable_if_t<(cereal::traits::is_output_serializable<U,Archive>::value&&cereal::traits::is_input_serializable<U,Archive>::value),int> = 0> 
		void serialize( Archive &archive, void* address ) 
		{ 
			try { 
				archive( *static_cast<U*>( address ) ); 
			}
			catch( const std::exception &exc ) {}
		}
		template<typename Archive, typename U,std::enable_if_t<!(cereal::traits::is_output_serializable<U,Archive>::value&&cereal::traits::is_input_serializable<U,Archive>::value),int> = 0> void serialize( Archive &archive, U* t ) {} // no-op
	#endif

		std::function<void(void*)> mDestructor;
		std::function<void(void*)> mPreBuild;
		std::function<void(void*)> mPostBuild;
	};

	std::map<std::type_index,rt::ModulePtr> mModules;
	std::map<std::type_index,std::vector<void*>> mInstances;
	std::map<std::type_index,Callbacks> mCallbacks;
};

template<typename T>
void ClassWatcher::Callbacks::init()
{
	mDestructor = [this]( void* address ) {
		static_cast<T*>( address )->~T();
	};
	mPreBuild = [this]( void* address ) {
		callPreBuildMethod<T>( static_cast<T*>( address ) );
	};
	mPostBuild = [this]( void* address ) {
		callPostBuildMethod<T>( static_cast<T*>( address ) );
	};
}

template<typename T>
void ClassWatcher::initCallbacks( const std::type_index &typeIndex )
{
	if( ! mCallbacks.count( typeIndex ) ) {
		mCallbacks[typeIndex].init<T>();
	}
}

template<typename T>
void ClassWatcher::watch( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, rt::BuildSettings settings )
{
	initCallbacks<T>( typeIndex );
	watchImpl( typeIndex, address, name, filePaths, dllPath, settings );
}

class ClassWatcherException : public ci::Exception {
public:
	ClassWatcherException( const std::string &message ) : ci::Exception( message ) {}
};

// --------------------------------------------------------------
// Macro helper routines
// --------------------------------------------------------------

namespace details {
	static std::string stripNamespace( const std::string &className )
	{
		auto pos = className.find_last_of( "::" );
		if( pos == std::string::npos )
			return className;

		std::string result = className.substr( pos + 1, className.size() - pos - 1 );
		return result;
	}

	static ci::fs::path makeDllPath( const ci::fs::path &intermediatePath, const char *className )
	{
		auto strippedClassName = stripNamespace( className );
		return intermediatePath / "runtime" / strippedClassName / "build" / ( strippedClassName + ".dll" );
	}

	template<class Class>
	void *makeAndAddClassWatcher( size_t size, const char *fileMacro, const char *className, rt::BuildSettings *settings )
	{
		void * ptr = ::operator new( size );
		auto headerPath = ci::fs::absolute( ci::fs::path( fileMacro ) );
		std::vector<ci::fs::path> sources;
		if( ci::fs::exists( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) ) ) {
			sources.push_back( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) );
		}
		sources.push_back( headerPath );

		if( ! settings ) {
			auto buildSettings = rt::BuildSettings().vcxproj();
			rt::ClassWatcher::instance().watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, sources, makeDllPath( buildSettings.getIntermediatePath(), className ), buildSettings );
		}
		else {
			rt::ClassWatcher::instance().watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, sources, makeDllPath( settings->getIntermediatePath(), className ), *settings );
		}
		return ptr;
	}

	template<class Class>
	void *makeAndAddClassWatcherWithHeader( size_t size, const char *fileMacro, const char *className, const ci::fs::path &headerPath, rt::BuildSettings *settings )
	{
		void * ptr = ::operator new( size );
		auto cppPath = ci::fs::absolute( fileMacro );
		if( ! settings ) {
			auto buildSettings = rt::BuildSettings().vcxproj();
			rt::ClassWatcher::instance().watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, { cppPath, headerPath }, makeDllPath( buildSettings.getIntermediatePath(), className ), buildSettings );
		}
		else {
			rt::ClassWatcher::instance().watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, { cppPath, headerPath }, makeDllPath( settings->getIntermediatePath(), className ), *settings );
		}
		return ptr;
	}
} // namespace details

} // namespace runtime

namespace rt = runtime;

#include "cinder/app/App.h"

#define RT_DECL \
public: \
	void* operator new( size_t size ); \
	void operator delete( void* ptr ); \
private: \
	static const ci::fs::path __rt_getHeaderPath() { return ci::fs::absolute( ci::fs::path( __FILE__ ) ); } \
public:



#define __RT_IMPL1( Class ) \
void* Class::operator new( size_t size ) \
{ \
	return rt::details::makeAndAddClassWatcherWithHeader<Class>( size, __FILE__, #Class, __rt_getHeaderPath(), nullptr );\
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::ClassWatcher::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL2( Class, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	return rt::details::makeAndAddClassWatcherWithHeader<Class>( size, __FILE__, #Class, __rt_getHeaderPath(), &Settings );\
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::ClassWatcher::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL3( Class, Header, Source, Dll, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	rt::ClassWatcher::instance().watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), std::string( #Class ), { Source, Header }, Dll, Settings );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::ClassWatcher::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \


#define __RT_IMPL_INLINE1( Class ) \
public: \
void* operator new( size_t size ) \
{ \
	return rt::details::makeAndAddClassWatcher( size, __FILE__, #Class, nullptr );\
} \
void operator delete( void* ptr ) \
{ \
	rt::ClassWatcher::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL_INLINE2( Class, Settings ) \
public: \
void* operator new( size_t size ) \
{ \
	return rt::details::makeAndAddClassWatcher( size, __FILE__, #Class, &Settings );\
} \
void operator delete( void* ptr ) \
{ \
	rt::ClassWatcher::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL_SWITCH(_1,_2,_3,NAME,...) NAME
#define __RT_IMPL_INLINE_SWITCH(_1,_2,NAME,...) NAME
#define __RT_IMPL_EXPAND(x) x
#define RT_IMPL( ... ) __RT_IMPL_EXPAND(__RT_IMPL_SWITCH(__VA_ARGS__,__RT_IMPL3,__RT_IMPL2,__RT_IMPL1))__RT_IMPL_EXPAND((__VA_ARGS__))
#define RT_IMPL_INLINE( ... ) __RT_IMPL_EXPAND(__RT_IMPL_INLINE_SWITCH(__VA_ARGS__,__RT_IMPL_INLINE2,__RT_IMPL_INLINE1))__RT_IMPL_EXPAND((__VA_ARGS__))

#else

#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"
#define RT_DECL \
private: \
	static const ci::fs::path __rt_getHeaderPath() { return ci::fs::absolute( ci::fs::path( __FILE__ ) ); } \
public:
#define RT_IMPL( ... )
#define RT_IMPL_INLINE( ... )

#endif