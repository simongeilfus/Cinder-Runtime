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
/*
TODO:
	[ ] include Class.cpp in ClassFactory.cpp and only build the later
	[ ] change module name / type name / target name?
	[ ] cache module fn ptrs to Type std::functions

*/
#pragma once

#include <typeindex>

#include "cinder/Exception.h"
#include "cinder/Filesystem.h"
#include "cinder/FileWatcher.h"
#include "cinder/Noncopyable.h"

#include "runtime/Export.h"
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

class CI_RT_API Factory : public ci::Noncopyable {
public:
	//! Returns the global Factory instance
	static Factory& instance();

	//! Allocates a new instance
	template<class Class>
	void* allocate();
	//! Allocates a new instance
	void* allocate( size_t size, const std::type_index &typeIndex );
	
	//! TypeFormat allows to opt-in or out from code generation and symbol exports
	class CI_RT_API TypeFormat {
	public:
		TypeFormat() : mPrecompiledHeader( true ), mClassFactory( true ), mExportVftable( true ) {}
		//! Adds a pre-build step to generate the precompiled header sources and build settings
		TypeFormat& precompiledHeader( bool generate = true );
		//! Adds a pre-build step to generate the class factory sources and build settings
		TypeFormat& classFactory( bool generate = true );
		//! Adds a pre-build step to generate .def file exporting the class vtable
		TypeFormat& exportVftable( bool exportSymbol = true );
	protected:
		friend class Factory;
		bool mPrecompiledHeader;
		bool mClassFactory;
		bool mExportVftable;
	};

	//! Allocates a new instance and adds it to the Factory watch list
	template<class Class>
	void* allocateAndWatch( size_t size, const char* sourceFilename, const char* className, rt::BuildSettings* settings, const TypeFormat &format = TypeFormat() );
	//! Allocates a new instance and adds it to the Factory watch list
	template<class Class>
	void* allocateAndWatch( size_t size, const char* sourceFilename, const char* className, const ci::fs::path &headerPath, rt::BuildSettings* settings, const TypeFormat &format = TypeFormat() );
	
	//! Adds an instance to the Factory watch list
	template<typename T>
	void watch( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, rt::BuildSettings settings = rt::BuildSettings().vcxproj(), const TypeFormat &format = TypeFormat() );
	//! Removes an instance from Factory watch list
	void unwatch( const std::type_index &typeIndex, void* address );

protected:
	template<typename T>
	void initType( const std::type_index &typeIndex, const std::string &name, const ci::fs::path &dllPath );
	void watchImpl( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, rt::BuildSettings settings = rt::BuildSettings().vcxproj(), const TypeFormat &format = TypeFormat() );
	void sourceChanged( const ci::WatchEvent &event, const std::type_index &typeIndex, const std::vector<ci::fs::path> &filePaths, const rt::BuildSettings &settings );
	void handleBuild( const rt::BuildOutput &output, const std::type_index &typeIndex, const std::string &vtableSym );
	void swapInstancesVtables( const std::type_index &typeIndex, const std::string &vtableSym );
	void reconstructInstances( const std::type_index &typeIndex );
	
	std::string stripNamespace( const std::string &className );
	ci::fs::path makeDllPath( const ci::fs::path &intermediatePath, const char *className );

	class Type {
	public:
		template<typename T>
		void init( const std::string &name, const ci::fs::path &dllPath );
		
		const std::function<void()>&		getNew() const { return mNew; }
		const std::function<void(void*)>&	getPlacementNew() const { return mPlacementNew; }
		const std::function<void(void*)>&	getDestructor() const { return mDestructor; }
		const std::function<void(void*)>&	getPreBuild() const { return mPreBuild; }
		const std::function<void(void*)>&	getPostBuild() const { return mPostBuild; }
		
		const rt::ModulePtr&	getModule() const { return mModule; }
		std::string				getName() const { return mName; }

		const std::vector<void*>&	getInstances() const { return mInstances; }
		std::vector<void*>&			getInstances() { return mInstances; }
		
		void setNew( const std::function<void()> &fn ) { mNew = fn; }
		void setPlacementNew( const std::function<void(void*)> &fn) { mPlacementNew = fn; }

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
		
		rt::ModulePtr				mModule;
		std::vector<void*>			mInstances;
		std::string					mName;

		std::function<void()>		mNew;
		std::function<void(void*)>	mPlacementNew;
		std::function<void(void*)>	mDestructor;
		std::function<void(void*)>	mPreBuild;
		std::function<void(void*)>	mPostBuild;
	};

	std::map<std::type_index,Type> mTypes;
};

template<typename T>
void Factory::Type::init( const std::string &name, const ci::fs::path &dllPath )
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
	mModule = std::make_unique<rt::Module>( dllPath );
	mName = name;
}

template<typename T>
void Factory::initType( const std::type_index &typeIndex, const std::string &name, const ci::fs::path &dllPath )
{
	if( ! mTypes.count( typeIndex ) ) {
		mTypes[typeIndex].init<T>( name, dllPath );
	}
}

template<typename T>
void Factory::watch( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const ci::fs::path &dllPath, rt::BuildSettings settings, const TypeFormat &format )
{
	initType<T>( typeIndex, name, dllPath );
	watchImpl( typeIndex, address, name, filePaths, dllPath, settings, format );
}

template<class Class>
void* Factory::allocateAndWatch( size_t size, const char* sourceFilename, const char* className, rt::BuildSettings* settings, const TypeFormat &format )
{
	void* ptr = allocate<Class>();
	auto headerPath = ci::fs::absolute( ci::fs::path( sourceFilename ) );
	std::vector<ci::fs::path> sources;
	if( ci::fs::exists( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) ) ) {
		sources.push_back( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) );
	}
	sources.push_back( headerPath );

	if( ! settings ) {
		auto buildSettings = rt::BuildSettings().vcxproj();
		watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, sources, makeDllPath( buildSettings.getIntermediatePath(), className ), buildSettings, format );
	}
	else {
		watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, sources, makeDllPath( settings->getIntermediatePath(), className ), *settings, format );
	}
	return ptr;
}

template<class Class>
void* Factory::allocateAndWatch( size_t size, const char* sourceFilename, const char* className, const ci::fs::path &headerPath, rt::BuildSettings* settings, const TypeFormat &format )
{
	void* ptr = allocate<Class>();
	auto cppPath = ci::fs::absolute( sourceFilename );
	if( ! settings ) {
		auto buildSettings = rt::BuildSettings().vcxproj();
		watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, { cppPath, headerPath }, makeDllPath( buildSettings.getIntermediatePath(), className ), buildSettings, format );
	}
	else {
		watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), className, { cppPath, headerPath }, makeDllPath( settings->getIntermediatePath(), className ), *settings, format );
	}
	return ptr;
}
template<class Class>
void* Factory::allocate()
{
	return allocate( sizeof(Class), std::type_index(typeid(Class)) );
}

class FactoryException : public ci::Exception {
public:
	FactoryException( const std::string &message ) : ci::Exception( message ) {}
};

} // namespace runtime

namespace rt = runtime;

#if defined( CINDER_SHARED )

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
	return rt::Factory::instance().allocateAndWatch<Class>( size, __FILE__, #Class, __rt_getHeaderPath(), nullptr );\
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::Factory::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL2( Class, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	return rt::Factory::instance().allocateAndWatch<Class>( size, __FILE__, #Class, __rt_getHeaderPath(), &Settings );\
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::Factory::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL3( Class, Header, Source, Dll, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	rt::Factory::instance().watch<Class>( std::type_index(typeid(Class)), static_cast<Class*>( ptr ), std::string( #Class ), { Source, Header }, Dll, Settings );\
	return ptr; \
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::Factory::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \


#define __RT_IMPL_INLINE1( Class ) \
public: \
void* operator new( size_t size ) \
{ \
	return rt::Factory::instance().allocateAndWatch( size, __FILE__, #Class, nullptr );\
} \
void operator delete( void* ptr ) \
{ \
	rt::Factory::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL_INLINE2( Class, Settings ) \
public: \
void* operator new( size_t size ) \
{ \
	return rt::Factory::instance().allocateAndWatch( size, __FILE__, #Class, &Settings );\
} \
void operator delete( void* ptr ) \
{ \
	rt::Factory::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL_SWITCH(_1,_2,_3,NAME,...) NAME
#define __RT_IMPL_INLINE_SWITCH(_1,_2,NAME,...) NAME
#define __RT_IMPL_EXPAND(x) x
#define RT_IMPL( ... ) __RT_IMPL_EXPAND(__RT_IMPL_SWITCH(__VA_ARGS__,__RT_IMPL3,__RT_IMPL2,__RT_IMPL1))__RT_IMPL_EXPAND((__VA_ARGS__))
#define RT_IMPL_INLINE( ... ) __RT_IMPL_EXPAND(__RT_IMPL_INLINE_SWITCH(__VA_ARGS__,__RT_IMPL_INLINE2,__RT_IMPL_INLINE1))__RT_IMPL_EXPAND((__VA_ARGS__))

#else

#define RT_DECL
#define RT_IMPL( ... )
#define RT_IMPL_INLINE( ... )

#endif