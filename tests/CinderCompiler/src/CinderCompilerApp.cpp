#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "runtime/Memory.h"

#include "Test.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CinderCompilerApp : public App {
 public:
	CinderCompilerApp();
	void draw() override;

	rt::shared_ptr<Test> mTest;
};


// (compiler) Debug Information Format: Program Database /Zi	
// (compiler) /MP No tangible benefits
// (compiler) /FS not really needed / working
// (linker) /INCREMENTAL:NO as we're changing the .pdb file before building there's no need for this (performing full link anyway)
// (linker) /OPT:NOLBR
CinderCompilerApp::CinderCompilerApp()
{
	mTest = rt::make_shared<Test>();
}

void CinderCompilerApp::draw()
{
	if( ! mTest ) {
		gl::clear( Color( 0, 0, 0 ) ); 
	}
	else if( mTest ){
		mTest->clear();
		mTest->render();
	}
}

CINDER_APP( CinderCompilerApp, RendererGl )