#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Basics.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BasicsApp : public App {
public:
	void setup() override;
	void draw() override;
	
	unique_ptr<Basics> mTest;
};

void BasicsApp::setup()
{
	mTest = make_unique<Basics>();
}

void BasicsApp::draw()
{
	gl::clear();

	mTest->draw();
}

CINDER_APP( BasicsApp, RendererGl() )