#include "Basics.h"

// when not opting-out from the runtime compiler
// precompiled header options, all the header will
// be added to a precompiled header to make future
// rebuild (much) faster. This makes making changes 
// to include the first or the first compilation longer
#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/wrapper.h"

using namespace std;
using namespace ci;
using namespace ci::app;

void Basics::draw()
{
	gl::clear( Color::gray( 0.8f ) );
}

// RT_IMPL has to be added to mark the class
// as runtime reloadable. a rt::Compiler::BuildSettings
// can be added as the second paramater of the macro to
// specify custom settings, include paths, libraries, ...
// Modifying the cpp file is usually much faster
RT_IMPL( Basics );