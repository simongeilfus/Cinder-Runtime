#include "Test.h"

#include "cinder/Cinder.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/wrapper.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/scoped.h"
#include "cinder/Font.h"
#include "cinder/app/App.h"

using namespace std;
using namespace ci;

Test::Test()
{
	mFont = Font( "Arial", 45.0f );
}

void Test::clear() const 
{
	ci::gl::clear( ci::ColorA( 0, 0, 0, 1 ) );
}
void Test::render() const 
{
	gl::ScopedBlendAlpha scopedBlend;
	gl::drawStringCentered( "Runtime Compiled " + getText(), app::getWindowCenter() - vec2( 0, mFont.getSize() * 0.5f ), ColorA::white(), mFont );	
}
std::string Test::getText() const 
{
	return "C++"; 
}