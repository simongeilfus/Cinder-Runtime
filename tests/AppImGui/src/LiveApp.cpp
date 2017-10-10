#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "CinderImGui.h"
#include "runtime/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LiveApp : public App {
public:
	LiveApp();
	void setup() override;
	void draw() override;

	CameraPersp mCamera;
	CameraUi mCameraUi;
	gl::BatchRef mBatch;
};

LiveApp::LiveApp()
: mCamera( CameraPersp( getWindowWidth(), getWindowHeight(), 60.0f, 0.1f, 100.0f ).calcFraming( Sphere( vec3( 0.0f ), 10.0f ) ) ),
mCameraUi( &mCamera, getWindow(), -1 )
{
	ui::initialize();
}

const string vert = R"(
uniform mat4 ciModelViewProjection;
in vec4 ciPosition;
void main( void )
{
	gl_Position = ciModelViewProjection * ciPosition;
}
)";
const string frag = R"(
out vec4 oColor;
void main( void )
{
	oColor = vec4( 1, 1, 1, 1 );
}
)";

void LiveApp::setup()
{
	auto glslProg = gl::GlslProg::create( vert, frag );
	mBatch = gl::Batch::create( geom::Cube(), glslProg );
}

void LiveApp::draw()
{
	static Color color;
	ui::ColorEdit3( "Background", &color[0] );
	gl::clear( color );
	gl::setMatrices( mCamera );
	mBatch->draw();
}

CINDER_RUNTIME_APP( LiveApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )