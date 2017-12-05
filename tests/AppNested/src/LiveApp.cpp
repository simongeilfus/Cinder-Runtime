#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"

#include "runtime/App.h"
#include "Nested.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LiveApp : public App {
public:
	LiveApp();
	void setup() override;
	void draw() override;

	unique_ptr<Nested> mNested;
};

LiveApp::LiveApp()
{
	mNested = make_unique<Nested>();
}

void LiveApp::setup()
{
	// if instanciated here, will use Nested.obj instead of Nested.dll
	// mNested = make_unique<Nested>();
}

void LiveApp::draw()
{
	gl::clear();
	mNested->draw();
}

CINDER_RUNTIME_APP( LiveApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )