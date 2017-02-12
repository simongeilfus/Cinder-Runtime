#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "runtime/Memory.h"

#include "Test.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BasicsApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;

	rt::shared_ptr<Test> mTest;
};

void BasicsApp::setup()
{
	mTest = rt::make_shared<Test>();
}

void BasicsApp::update()
{
	if( mTest ) {
		mTest->update();
	}
}

void BasicsApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 

	if( mTest ) {
		mTest->draw();
	}
}

CINDER_APP( BasicsApp, RendererGl )
