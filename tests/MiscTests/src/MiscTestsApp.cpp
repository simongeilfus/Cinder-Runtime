#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "TypeWithNamespace.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MiscTestsApp : public App {
public:
	void setup() override;
	void draw() override;
	
	unique_ptr<foo::bar::TypeWithNamespace> mTest;
};

void MiscTestsApp::setup()
{
	mTest = make_unique<foo::bar::TypeWithNamespace>();
}

void MiscTestsApp::draw()
{
	gl::clear();
	
	mTest->draw();
}

CINDER_APP( MiscTestsApp, RendererGl() )
