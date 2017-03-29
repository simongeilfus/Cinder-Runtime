#include "Test.h"
#include "cinder/app/App.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/gl/wrapper.h"
#include "cinder/gl/scoped.h"

using namespace std;
using namespace ci;
using namespace ci::app;

Test::Test()
{
	mFont = gl::TextureFont::create( Font( "Arial", 35 ) );
	mBatch = gl::Batch::create( geom::Circle().radius( 8.5f ).subdivisions( 32 ), gl::getStockShader( gl::ShaderDef().color() ) );
}

void Test::update()
{
}

void Test::draw()
{
	gl::clear( ColorA::gray( 0.85f ) );
	
	gl::ScopedColor textColor( ColorA::gray( 0.25f ) );
	auto text = "Runtime Compiled c++";
	auto textSize = mFont->measureString( text );
	mFont->drawString( text, getWindowCenter() - vec2( textSize.x * 0.5f, 0.0f ) );
	
	gl::ScopedColor batchColor( ColorA::gray( 0.95f ) );
	gl::ScopedModelMatrix scopedModel;
	gl::translate( getWindowCenter() - vec2( textSize.x * 0.5f + 25.0f, 9.0f ) );
	mBatch->draw();
}
