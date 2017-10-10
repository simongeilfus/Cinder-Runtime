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
	
	template<class Archive>
	void serialize( Archive &archive )
	{
	    console() << "test" << endl;
		// archive( mTest );
	}
	float mTest;
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
	ui::DragFloat( "test", &mTest );
	gl::clear( Color::gray( 0.75f ) );
}

CINDER_RUNTIME_APP( LiveApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )