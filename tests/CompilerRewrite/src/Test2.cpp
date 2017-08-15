#include "Test2.h"

const char* Test2::getString()
{
	return "Hello!";
}


// 
#if ! defined( RT_COMPILED ) && defined( CINDER_SHARED )

#include "runtime/ClassInstanceWatcher.h"

void* Test2::operator new( size_t size )
{
	void * ptr = ::operator new( size );
	rt::watchClassInstance( static_cast<Test2*>( ptr ), { CI_RT_PROJECT_ROOT / "src/Test2.cpp", CI_RT_PROJECT_ROOT / "src/Test2.h" }, CI_RT_INTERMEDIATE_DIR / "runtime/Test2/Test2.dll" );
	return ptr;
}
	
void Test2::operator delete( void* ptr )
{
	rt::watchClassInstance( static_cast<Test2*>( ptr ), {}, "", true );
	::operator delete( ptr );
}
#endif
