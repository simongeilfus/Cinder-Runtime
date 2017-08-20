#include "Test2.h"
#include "cinder/Area.h"
#include "cinder/gl/Shader.h"

using namespace std;
using namespace ci;

Test2::Test2()
: mCount( 0 )
{
	mCount = 10;
	mBatch = gl::Batch::create( geom::Rect( Rectf( vec2(0), vec2(10) ) ), gl::getStockShader( gl::ShaderDef().color() ) );
}

std::string Test2::getString()
{

	mCount++;
	return ( "_" + std::to_string( mCount ) );
}


void Test2::draw()
{
	gl::translate( vec2(120,220) );
	mBatch->draw();
}

RT_WATCH_IMPL( Test2 )