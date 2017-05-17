#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "runtime/CompilerMSVC.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CompilerRewriteApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;

	rt::CompilerRef mCompiler;
};

void CompilerRewriteApp::setup()
{
	mCompiler = rt::Compiler::create();
}

void CompilerRewriteApp::update()
{
}

void CompilerRewriteApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( CompilerRewriteApp, RendererGl() )
