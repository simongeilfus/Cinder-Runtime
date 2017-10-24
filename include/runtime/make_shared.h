#pragma once

namespace runtime {

#if defined( CINDER_SHARED )

template<class T,class ...Args>
inline std::shared_ptr<T> make_shared( Args&&... args )
{
	return std::shared_ptr<T>( new T( std::forward<Args>( args )... ) );
}

#else

template<class T,class ...Args>
inline std::shared_ptr<T> make_shared( Args&&... args )
{
	return std::make_shared<T>( std::forward<Args>( args )... );
}

#endif

} // namespace runtime

namespace rt = runtime;