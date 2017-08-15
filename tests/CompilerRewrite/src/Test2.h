#pragma once

#include <string>

class Test2 {
public:
	Test2();
	virtual std::string getString();

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )
	void* operator new( size_t size );
	void operator delete( void* ptr );
#endif

};