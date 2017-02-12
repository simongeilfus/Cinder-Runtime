#include "Test.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

using namespace std;
using namespace ci;
using namespace ci::app;

Test::Test()
{
	mBatch = gl::Batch::create( geom::Circle().center( getWindowCenter() ).radius( 40.0f ).subdivisions( 32 ), gl::getStockShader( gl::ShaderDef().color() ) );
}

void Test::update()
{
}

void Test::draw()
{
	gl::clear( ColorA::gray( 0.85f ) );
	gl::ScopedColor scopedColor( ColorA::gray( 0.95f ) );
	mBatch->draw();
}
