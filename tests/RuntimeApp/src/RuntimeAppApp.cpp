#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "runtime/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class RuntimeAppApp : public rt::App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void RuntimeAppApp::setup()
{
	//console() << "Test324" << endl;
}

void RuntimeAppApp::mouseDown( MouseEvent event )
{
}

void RuntimeAppApp::update()
{
}

void RuntimeAppApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( RuntimeAppApp, RendererGl )
