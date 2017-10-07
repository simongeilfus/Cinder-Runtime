#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// This tests requires Cinder-ImGui to be installed in the same
// directory as Cinder-Runtime. Because Dear-ImGui uses static objects
// Cinder-ImGui needs to be built as a dll. (see Cinder-ImGui/proj/vc2015)
#include "CinderImGui.h"

#include "Test.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ImGuiApp : public App {
public:
	void setup() override;
	void draw() override;
	
	// any runtime reloadable instances have to be heap allocated
	// but which flavor doesn't really matter. (std::shared_ptrs, 
	// std::unique_ptrs, raw pointers, ... will all work)
	unique_ptr<Test> mTest;
};

void ImGuiApp::setup()
{
	// initialize Dear-ImGui
	ui::initialize();

	// there's no difference at instantiation at the exception 
	// of shared_ptr that needs to be instantiated by calling 
	// rt::make_shared or by using the class new operator. The
	// first approached is prefered as it will be turned into a
	// regular std::make_shared when building in Release or Debug.
	mTest = make_unique<Test>();
}

void ImGuiApp::draw()
{
	gl::clear();
	
	// any function marked as virtual will be hot-swapped at runtime
	mTest->draw();
}

CINDER_APP( ImGuiApp, RendererGl() )