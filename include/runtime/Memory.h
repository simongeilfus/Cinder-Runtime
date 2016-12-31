#pragma once

#include <map>
#include <memory>

#if ! defined( WIN32_LEAN_AND_MEAN )
	#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include "cinder/Filesystem.h"
#include "cinder/Log.h"
#include "cinder/Signals.h"
#include "cinder/System.h"

#include "runtime/Module.h"

namespace runtime {

#if ! defined( RT_DISABLE_SHARED_POINTERS )

namespace details {
	template<typename T> class Class;
}

template<class T>
class shared_ptr {
public:
	constexpr shared_ptr() {}
	
	operator bool() const { return mPtr.operator bool(); }
	T* operator->() const { return mPtr.operator->(); }
	T* get() const { return mPtr.get(); }
	long use_count() const { return mPtr.use_count(); }
	void reset() { return mPtr.reset(); }
	
	shared_ptr( const shared_ptr& other );
	shared_ptr( shared_ptr&& other );
	shared_ptr& operator=( const shared_ptr& other );
	shared_ptr& operator=( shared_ptr&& other );
	~shared_ptr();
	
	static void registerClass( const ci::fs::path &path );

protected:
	void update( const std::shared_ptr<T> &newInstance );
	
	template<class... Args>
	shared_ptr( bool runtime, Args&&... args )
	: mPtr( std::make_shared<T>( std::forward<Args>( args )... ) )
	{
		details::Class<T>::registerInstance( this );
	}

	template<class U, class... Args>
	friend shared_ptr<U> make_shared( Args&&... args );
	friend class details::Class<T>;
	
	std::shared_ptr<T> mPtr;
};

template<class T, class... Args>
inline shared_ptr<T> make_shared( Args&&... args ) 
{ 
	return shared_ptr<T>( true, std::forward<Args>( args )... ); 
}

namespace details {
	template<typename T>
	class Class {
	public:
		static void init( const ci::fs::path &path = ci::fs::path() );
		static bool isInitialized();
		static void unregisterInstance( shared_ptr<T>* inst );
		static void registerInstance( shared_ptr<T>* inst );
		static Class* get();
		static ci::fs::path locateSources( const ci::fs::path &path );
		
		typedef void (__cdecl* FactoryPtr)(std::shared_ptr<T>*);
		//typedef std::shared_ptr<T> (*FactoryPtr)(void);
		std::map<shared_ptr<T>*,ci::signals::ScopedConnection> mInstancesUpdate;
		std::map<shared_ptr<T>*,ci::signals::ScopedConnection> mInstancesCleanup;
		ModuleRef mModule;
	};
}

template<class T>
shared_ptr<T>::shared_ptr( const shared_ptr& other )
: mPtr( other.mPtr )
{
	details::Class<T>::registerInstance( this );
}

template<class T>
shared_ptr<T>::shared_ptr( shared_ptr&& other )
: mPtr( std::move( other.mPtr ) )
{
	// TODO : Do we realy need to unregister in case of a move?
	details::Class<T>::unregisterInstance( &other );
	details::Class<T>::registerInstance( this );
}

template<class T>
shared_ptr<T>& shared_ptr<T>::operator=( const shared_ptr& other )
{
	details::Class<T>::registerInstance( this );
	mPtr = other.mPtr;
	return *this;
}
template<class T>
shared_ptr<T>& shared_ptr<T>::operator=( shared_ptr&& other )
{
	details::Class<T>::unregisterInstance( &other );
	details::Class<T>::registerInstance( this );
	mPtr = std::move( other.mPtr );
	return *this;
}

template<class T>
shared_ptr<T>::~shared_ptr()
{
	details::Class<T>::unregisterInstance( this );
}

template<class T>
void shared_ptr<T>::update( const std::shared_ptr<T> &newInstance )
{
	mPtr = newInstance;
}

template<typename T>
void shared_ptr<T>::registerClass( const ci::fs::path &path )
{
	details::Class<T>::init( path );
}

namespace details {

	template<typename T>
	typename Class<T>* Class<T>::get()
	{
		static std::unique_ptr<Class<T>> sClass = std::make_unique<Class<T>>();
		return sClass.get();
	}

	template<typename T>
	bool Class<T>::isInitialized()
	{
		return Class<T>::get()->mModule != nullptr;
	}
	
	template<typename T>
	ci::fs::path Class<T>::locateSources( const ci::fs::path &path )
	{
		auto appPath = ci::app::getAppPath();
		auto appPathStr = appPath.string();
		auto projectPath = appPath.parent_path().parent_path().parent_path();
		if( ci::fs::is_directory( projectPath ) ) {
			ci::fs::recursive_directory_iterator dir( projectPath ), endDir;
			for( ; dir != endDir; ++dir ) {
				auto current = (*dir).path();
				if( current.string().find( appPathStr ) == std::string::npos ) {
					if( current.filename() == path ) {
						return ci::fs::canonical( current );
					}
				}
			}
		}
		return ci::fs::path();
	}

	template<typename T>
	void Class<T>::init( const ci::fs::path &path )
	{
		auto c = Class<T>::get();
		if( ! path.empty() ) {
			c->mModule = ModuleManager::get()->add( path );
		}
		// try to locate source
		else {
			auto className = ci::System::demangleTypeName( typeid(T).name() );
			if( className.find( "class " ) != std::string::npos ) {
				className = className.substr( 6 );
			}
			auto cppPath = locateSources( className + ".cpp" );
			if( ! cppPath.empty() ) {
				c->mModule = ModuleManager::get()->add( cppPath );				
			}
			else {
				auto headerPath = locateSources( className + ".h" );
				if( ! headerPath.empty() ) {
					c->mModule = ModuleManager::get()->add( headerPath );
				}
			}

			if( ! c->isInitialized() ) {
				CI_LOG_E( "Can't locate " << className << " source path. Please use rt::shared_ptr<T>::registerClass." );
			}
		}
	}

	template<typename T>
	void Class<T>::registerInstance( shared_ptr<T>* inst )
	{
		if( ! Class<T>::isInitialized() ) {
			Class<T>::init();
		}
		if( Class<T>::isInitialized() ) {
			// connect to the cleanup signals to allow releasing old pointer before releasing library
			Class<T>::get()->mInstancesCleanup[inst] = Class<T>::get()->mModule->getCleanupSignal().connect( [inst]( const ModuleRef &module ) {
				inst->reset();
			} );
			// connect to the new module signal
			Class<T>::get()->mInstancesUpdate[inst] = Class<T>::get()->mModule->getChangedSignal().connect( [inst]( const ModuleRef &module ) {
				if( auto handle = module->getHandle() ) {
					/*std::string factorySymbol = "";
					const auto &symbols = module->getSymbols();
					for( const auto &symbol : symbols ) {
						if( ( symbol.first.find( "shared_ptr" ) != std::string::npos
							&& symbol.first.find( "runtimeCreateFactory" ) != std::string::npos ) ||
							( symbol.second.find( "shared_ptr" ) != std::string::npos
							&& symbol.second.find( "runtimeCreateFactory" ) != std::string::npos ) ) {
							factorySymbol = symbol.second;
						}
					}*/
					//if( ! factorySymbol.empty() ) {
						//if( auto factoryCreate = (FactoryPtr) GetProcAddress( static_cast<HMODULE>( handle ), factorySymbol.c_str() ) ) {
							//if( std::shared_ptr<T> newPtr = factoryCreate() ) {
							//	inst->update( newPtr );
							//}
						if( auto factoryCreate = (FactoryPtr) GetProcAddress( static_cast<HMODULE>( handle ), "runtimeCreateFactory" ) ) {
							std::shared_ptr<T> newPtr;
							factoryCreate( &newPtr );
							if( newPtr ) {
								inst->update( newPtr );
							}
						}
					//}
				}
			} );
		}
	}

	template<typename T>
	void Class<T>::unregisterInstance( shared_ptr<T>* inst )
	{
		if( Class<T>::isInitialized() ) {
			if( get()->mInstancesUpdate.count( inst ) ) {
				get()->mInstancesUpdate.erase( inst );
			}
			if( get()->mInstancesCleanup.count( inst ) ) {
				get()->mInstancesCleanup.erase( inst );
			}
		}
	}

} // namespace details
#else
template<class T>
using shared_ptr = std::shared_ptr<T>;
template<class T, class... Args>
inline std::shared_ptr<T> make_shared( Args&&... args ) 
{ 
	return std::make_shared<T>( std::forward<Args>( args )... ); 
}
#endif

} // namespace runtime

namespace rt = runtime;