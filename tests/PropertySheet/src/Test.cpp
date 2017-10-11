#include "Test.h"

#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/wrapper.h"

using namespace std;
using namespace ci;
using namespace ci::app;

Test::Test()
{
}

void Test::draw()
{
	gl::clear( Color( 0, 1, 0 ) );
}

RT_WATCH_IMPL( Test );