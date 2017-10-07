#include "Basics.h"
#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/wrapper.h"

using namespace std;
using namespace ci;
using namespace ci::app;

Basics::Basics()
{
}

void Basics::draw()
{
	gl::clear( Color::gray( 0.8f ) );
}

RT_WATCH_IMPL( Basics );