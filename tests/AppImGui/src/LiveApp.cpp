#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "CinderImGui.h"

#include "runtime/App.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/details/traits.hpp>
#include "CinderCereal.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LiveApp : public App {
public:
	LiveApp();
	void setup() override;
	void draw() override;
};

LiveApp::LiveApp()
{
	ui::initialize();
}

void LiveApp::setup()
{
}

void LiveApp::draw()
{
	static float gray = 0.75f;
	ui::DragFloat( "Background", &gray, 0.01f, 0.0f, 1.0f );
	gl::clear( Color::gray( gray ) );
}

CINDER_RUNTIME_APP( LiveApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )