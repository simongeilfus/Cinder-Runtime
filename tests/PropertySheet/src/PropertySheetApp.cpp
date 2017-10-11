#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Test.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PropertySheetApp : public App {
public:
	void setup() override;
	void draw() override;

	unique_ptr<Test> mTest;
};

void PropertySheetApp::setup()
{
	mTest = make_unique<Test>();
}

void PropertySheetApp::draw()
{
	gl::clear();

	// any function marked as virtual will be hot-swapped at runtime
	mTest->draw();
}

CINDER_APP( PropertySheetApp, RendererGl() )