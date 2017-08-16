#include "Test2.h"

RT_WATCH_CLASS_IMPL( Test2 );

Test2::Test2()
: mCount( 0 )
{
	mCount = 10;
}

std::string Test2::getString()
{
	mCount++;
	//return "Hello!";
	return ( "#" + std::to_string( mCount ) );
}

