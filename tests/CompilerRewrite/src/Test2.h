#pragma once

class Test2 {
public:
	virtual const char* getString();

#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )
	void* operator new( size_t size );
	void operator delete( void* ptr );
#endif

};