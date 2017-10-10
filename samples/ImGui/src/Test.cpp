#include "Test.h"

// when not opting-out from the runtime compiler
// precompiled header options, all the header will
// be added to a precompiled header to make future
// rebuild (much) faster. This makes making changes 
// to include the first or the first compilation longer
#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/wrapper.h"

// see comment in ImGuiApp.cpp about Cinder-ImGui
#include "CinderImGui.h"

using namespace std;
using namespace ci;
using namespace ci::app;

Test::Test()
{
}

void Test::draw()
{
	ui::ScopedWindow window( "Test" );
	static ColorA background = ColorA::gray( 0.75f );
	
	ui::ColorEdit4( "Background", &background[0] );

	gl::clear( background );
}

// RT_WATCH_IMPL has to be added to mark the class
// as runtime reloadable. 
RT_WATCH_IMPL( Test );