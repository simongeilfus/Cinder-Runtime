#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "runtime/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Win32App : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void Win32App::setup()
{
}

void Win32App::mouseDown( MouseEvent event )
{
}

void Win32App::update()
{
}

void Win32App::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_RUNTIME_APP( Win32App, RendererGl )
