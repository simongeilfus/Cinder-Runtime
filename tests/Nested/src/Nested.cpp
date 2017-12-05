#include "Nested.h"
#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/wrapper.h"

using namespace std;
using namespace ci;
using namespace ci::app;

Nested::Nested()
{
}

void Nested::draw()
{
	gl::clear( Color::gray( 0.08f ) );
}

// RT_IMPL has to be added to mark the class
// as runtime reloadable. a rt::Compiler::BuildSettings
// can be added as the second paramater of the macro to
// specify custom settings, include paths, libraries, ...
// Modifying the cpp file is usually much faster
RT_IMPL( Nested );