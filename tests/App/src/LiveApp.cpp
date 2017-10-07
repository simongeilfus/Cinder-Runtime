#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "runtime/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LiveApp : public App {
public:
	void draw() override;
};

void LiveApp::draw()
{
	gl::clear( Color::gray( 0.5f ) );
}

CINDER_RUNTIME_APP( LiveApp, RendererGl )