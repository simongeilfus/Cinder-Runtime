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
	[ ] Factory::Notification
	[ ] Nesting = Modify LinkAppObj to be if reloaded link import.lib otherwise class.obj
	[ ] Add error checking to codegen?

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

// If cereal is included before this file any serialization methods
// added to a class will be used to save states between reloads
#if defined( CEREAL_CEREAL_HPP_ )
#include <cereal/details/traits.hpp>
#include <cereal/archives/binary.hpp>
#endif

// This allow to change the name of the class method called before reloading happens
#if ! defined( RT_PRE_BUILD_METHOD_NAME )
#define RT_PRE_BUILD_METHOD_NAME cleanup
#endif

// This allow to change the name of the class method called after reloading happened
#if ! defined( RT_POST_BUILD_METHOD_NAME )
#define RT_POST_BUILD_METHOD_NAME setup
#endif

namespace runtime {

class CI_RT_API Factory : public ci::Noncopyable {
public:
	//! Returns the global Factory instance
	static Factory& instance();

	//! Allocates a new instance of the latest version of the Class
	template<class Class>
	void* allocate();
	
	//! TypeFormat allows to opt-in or out from code generation and symbol exports
	class CI_RT_API TypeFormat {
	public:
		TypeFormat() : mPrecompiledHeader( true ), mClassFactory( true ), mExportVftable( true ), mLinkAppObjs( true ) {}
		//! Adds a pre-build step to generate the precompiled header sources and build settings
		TypeFormat& precompiledHeader( bool generate = true );
		//! Adds a pre-build step to generate the class factory sources and build settings
		TypeFormat& classFactory( bool generate = true );
		//! Adds a pre-build step to generate .def file exporting the class vtable
		TypeFormat& exportVftable( bool exportSymbol = true );
		//! Adds the app's generated .obj files to be linked. Default to true
		TypeFormat& linkAppObjs( bool link );
	protected:
		friend class Factory;
		bool mPrecompiledHeader;
		bool mClassFactory;
		bool mExportVftable;
		bool mLinkAppObjs;
	};

	//! Allocates a new instance and adds it to the Factory watch list
	template<class Class>
	void* allocateAndWatch( const std::string &className, const ci::fs::path &headerPath, rt::BuildSettings* settings, const TypeFormat &format = TypeFormat() );
	//! Allocates a new instance and adds it to the Factory watch list
	template<class Class>
	void* allocateAndWatch( const std::string &className, const ci::fs::path &cppPath, const ci::fs::path &headerPath, rt::BuildSettings* settings, const TypeFormat &format = TypeFormat() );
	
	//! Adds an instance to the Factory watch list
	template<typename T>
	void watch( void* address, const std::string &className, const std::vector<ci::fs::path> &filePaths, const rt::BuildSettings &settings = rt::BuildSettings().vcxproj(), const TypeFormat &format = TypeFormat() );
	//! Removes an instance from Factory watch list
	void unwatch( const std::type_index &typeIndex, void* address );

	class CI_RT_API Type;
	Type* getType( const std::type_index &typeIndex );
	template<typename T> Type* getType() { return getType( std::type_index(typeid(T)) ); }

	const std::map<std::type_index,Type>&	getTypes() const { return mTypes; }
	std::map<std::type_index,Type>&			getTypes() { return mTypes; }

	class CI_RT_API Type {
	public:
		template<typename T>
		void init( const std::string &name );

		const std::function<void()>&		getNew() const { return mNew; }
		const std::function<void(void*)>&	getPlacementNew() const { return mPlacementNew; }
		const std::function<void(void*)>&	getDestructor() const { return mDestructor; }
		const std::function<void(void*)>&	getPreBuild() const { return mPreBuild; }
		const std::function<void(void*)>&	getPostBuild() const { return mPostBuild; }

		const rt::ModulePtr&	getModule() const { return mModule; }
		std::string				getName() const { return mName; }

		const std::vector<void*>&	getInstances() const { return mInstances; }
		std::vector<void*>&			getInstances() { return mInstances; }

		void setModule( rt::ModulePtr &&module ) { mModule = std::move( module ); }
		void setNew( const std::function<void()> &fn ) { mNew = fn; }
		void setPlacementNew( const std::function<void(void*)> &fn) { mPlacementNew = fn; }

		const std::type_index& getTypeIndex() const { return *mTypeIndex.get(); }

		class CI_RT_API Version {
		public:
			Version( size_t id, const ci::fs::path &path );

			size_t			getId() const { return mId; }
			ci::fs::path	getPath() const { return mPath; }

			std::chrono::system_clock::time_point getTimePoint() const { return mTimePoint; }
		protected:
			size_t			mId;
			ci::fs::path	mPath;
			std::chrono::system_clock::time_point mTimePoint;
		};

		const std::vector<Version>&	getVersions() const { return mVersions; }
		std::vector<Version>&		getVersions() { return mVersions; }

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

		std::vector<Version>		mVersions;
		std::unique_ptr<std::type_index> mTypeIndex;
	};

	void loadTypeVersion( const std::type_index &typeIndex, const Type::Version &version );

protected:
	template<typename T>
	void initType( const std::type_index &typeIndex, const std::string &name );
	void watchImpl( const std::type_index &typeIndex, void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, rt::BuildSettings settings = rt::BuildSettings().vcxproj(), const TypeFormat &format = TypeFormat() );
	void sourceChanged( const ci::WatchEvent &event, const std::type_index &typeIndex, const std::vector<ci::fs::path> &filePaths, const rt::BuildSettings &settings );
	void handleBuild( const rt::BuildOutput &output, const std::type_index &typeIndex, const std::string &vtableSym );
	void swapInstancesVtables( const std::type_index &typeIndex, const std::string &vtableSym );
	void reconstructInstances( const std::type_index &typeIndex );
	void* allocate( size_t size, const std::type_index &typeIndex );

	std::map<std::type_index,Type> mTypes;
};

template<typename T>
void Factory::Type::init( const std::string &name )
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
	mName = name;

	mTypeIndex = std::make_unique<std::type_index>( typeid(T) );
}

template<typename T>
void Factory::initType( const std::type_index &typeIndex, const std::string &name )
{
	if( ! mTypes.count( typeIndex ) ) {
		mTypes[typeIndex].init<T>( name );
	}
}

template<typename T>
void Factory::watch( void* address, const std::string &name, const std::vector<ci::fs::path> &filePaths, const rt::BuildSettings &settings, const TypeFormat &format )
{
	const std::type_index typeIndex = std::type_index(typeid(T));
	initType<T>( typeIndex, name );
	watchImpl( typeIndex, address, name, filePaths, settings, format );
}

template<class Class>
void* Factory::allocateAndWatch( const std::string &className, const ci::fs::path &header, rt::BuildSettings* settings, const TypeFormat &format )
{
	void* ptr = allocate<Class>();
	auto headerPath = ci::fs::absolute( ci::fs::path( header ) );
	std::vector<ci::fs::path> sources;
	if( ci::fs::exists( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) ) ) {
		sources.push_back( headerPath.parent_path() / ( headerPath.stem().string() + ".cpp" ) );
	}
	sources.push_back( headerPath );

	if( ! settings ) {
		auto buildSettings = rt::BuildSettings().vcxproj();
		watch<Class>( ptr, className, sources, buildSettings, format );
	}
	else {
		watch<Class>( ptr, className, sources, *settings, format );
	}
	return ptr;
}

template<class Class>
void* Factory::allocateAndWatch( const std::string &className, const ci::fs::path &cppPath, const ci::fs::path &headerPath, rt::BuildSettings* settings, const TypeFormat &format )
{
	void* ptr = allocate<Class>();
	if( ! settings ) {
		auto buildSettings = rt::BuildSettings().vcxproj();
		watch<Class>( ptr, className, { ci::fs::absolute( cppPath ), ci::fs::absolute( headerPath ) }, buildSettings, format );
	}
	else {
		watch<Class>( ptr, className, { ci::fs::absolute( cppPath ), ci::fs::absolute( headerPath ) }, *settings, format );
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
	return rt::Factory::instance().allocateAndWatch<Class>( #Class, __FILE__, __rt_getHeaderPath(), nullptr );\
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::Factory::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \

#define __RT_IMPL2( Class, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	return rt::Factory::instance().allocateAndWatch<Class>( #Class, __FILE__, __rt_getHeaderPath(), &Settings );\
} \
void Class::operator delete( void* ptr ) \
{ \
	rt::Factory::instance().unwatch( std::type_index(typeid(Class)), static_cast<Class*>( ptr ) ); \
	::operator delete( ptr ); \
} \
// TODO: Remove
#define __RT_IMPL3( Class, Header, Source, Dll, Settings ) \
void* Class::operator new( size_t size ) \
{ \
	void * ptr = ::operator new( size ); \
	rt::Factory::instance().watch<Class>( ptr, std::string( #Class ), { Source, Header }, Dll, Settings );\
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
	return rt::Factory::instance().allocateAndWatch<Class>( #Class, __FILE__, nullptr );\
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
	return rt::Factory::instance().allocateAndWatch<Class>( #Class, __FILE__, &Settings );\
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