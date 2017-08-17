#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Test2.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CompilerRewriteApp : public App {
public:
	void setup() override;
	void draw() override;
	
	vector<unique_ptr<Test2>> mTest;
};

void CompilerRewriteApp::setup()
{
	for( size_t i = 0; i < 1000; ++i ) {
		mTest.push_back( make_unique<Test2>() );
	}
}

void CompilerRewriteApp::draw()
{
	gl::clear();

	for( const auto &test : mTest ) {
		test->draw();
	}
}

CINDER_APP( CompilerRewriteApp, RendererGl() )