#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Test2.h"
#include "Clear.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CompilerRewriteApp : public App {
public:
	void setup() override;
	void draw() override;
	
	unique_ptr<Clear> mClear;
	vector<unique_ptr<Test2>> mTest;
};

void CompilerRewriteApp::setup()
{
	mClear = make_unique<Clear>();
	for( size_t i = 0; i < 100; ++i ) {
		mTest.push_back( make_unique<Test2>() );
	}
}

void CompilerRewriteApp::draw()
{
	mClear->clear();

	for( const auto &test : mTest ) {
		test->draw();
	}
}

CINDER_APP( CompilerRewriteApp, RendererGl() )