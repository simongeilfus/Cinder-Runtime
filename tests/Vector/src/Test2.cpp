#include "Test2.h"
#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/wrapper.h"
#include "cinder/Rand.h"

RT_WATCH_IMPL( Test2 );

using namespace std;
using namespace ci;
using namespace ci::app;

Test2::Test2()
{
	mColor = ColorA::gray( randFloat( 0.25f, 1.0f ) );
	mPosition = vec2( randFloat() * getWindowWidth(), randFloat() * getWindowHeight() );
	mSize = randFloat( 1, 10 );
}

void Test2::draw()
{
	gl::color( mColor );
	//gl::drawSolidCircle( mPosition, mSize * 0.75f );
	vec2 scl = vec2( 2.0f, 0.125f );
	gl::drawSolidRect( Rectf( mPosition - vec2( mSize ) * scl, mPosition + vec2( mSize ) * scl ) );
}

