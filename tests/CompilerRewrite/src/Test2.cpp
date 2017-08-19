#include "Test2.h"
#include "cinder/Area.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

RT_WATCH_IMPL( Test2 );

Test2::Test2()
: mCount( 0 )
{
	mCount = 10;
}

std::string Test2::getString()
{
	mCount++;
	//return "Hello!";
	return ( "___" + std::to_string( mCount ) );
}

