#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/FileWatcher.h"

#include "runtime/CompilerMsvc.h"
#include "runtime/Module.h"

#include "Test.h"
#include "Test2.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CompilerRewriteApp : public App {
public:
	void setup() override;
	void cleanup() override;
	void draw() override;
	
#if defined( CINDER_SHARED )
	void buildTestCpp();
	rt::ModulePtr mModule;
#endif
	
	unique_ptr<Test> mTest;



	unique_ptr<Test2> mTest2;
	shared_ptr<Test2> mTest3;
	Test2* mTest4;

	Font mFontLarge;
};

void CompilerRewriteApp::setup()
{
	setWindowPos( 0, 10 );
	mFontLarge = Font( "Arial", 35 );
	mTest = make_unique<Test>();
	mTest2 = make_unique<Test2>();
	mTest3 = shared_ptr<Test2>( new Test2 ); // make_shared won't work unfortunately
	mTest4 = new Test2();

	{
		auto test5 = make_unique<Test2>();
	}
	
#if defined( CINDER_SHARED )
	// init module and watch .h and .cpp
	mModule = make_unique<rt::Module>( CI_RT_INTERMEDIATE_DIR / "runtime/Test/Test.dll" );
	FileWatcher::instance().watch( 
		{ CI_RT_PROJECT_ROOT / "src/Test.h", CI_RT_PROJECT_ROOT / "src/Test.cpp" }, 
		FileWatcher::Options().callOnWatch( false ),
		[this]( const WatchEvent &event ) { buildTestCpp(); } 
	);
#endif
}

void CompilerRewriteApp::cleanup()
{
	delete mTest4;
}

void CompilerRewriteApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	
	if( mTest ) {
		gl::drawStringCentered( mTest->getString(), getWindowCenter(), ColorA::white(), mFontLarge );
	}
	
	if( mTest2 ) {
		gl::drawStringCentered( mTest2->getString(), getWindowCenter() - vec2( 0, 40 ), ColorA::white(), mFontLarge );
	}
	
	if( mTest3 ) {
		gl::drawStringCentered( mTest3->getString(), getWindowCenter() - vec2( 120, 40 ), ColorA::white(), mFontLarge );
	}
	
	if( mTest4 ) {
		gl::drawStringCentered( mTest4->getString(), getWindowCenter() - vec2( -120, 40 ), ColorA::white(), mFontLarge );
	}
	
}

#if defined( CINDER_SHARED )
void CompilerRewriteApp::buildTestCpp()
{
	Timer timer( true );

	// unlock the dll-handle before building
	mModule->unlockHandle();
	
	// prepare build settings
	auto settings = rt::Compiler::BuildSettings().define( "RT_COMPILED" )
		.default() // default cinder project settings
		.include( "../../../include" ); // cinder-runtime include folder
		
	// initiate the build
	rt::CompilerMsvc::instance().build( CI_RT_PROJECT_ROOT / "src/Test.cpp", settings, [=]( const rt::CompilationResult &result ) {
		// if a new dll exists update the handle
		if( fs::exists( mModule->getPath() ) ) {
			mModule->updateHandle();
			// and try to get a ptr to its make_unique factory function
			if( auto updatePtr = mModule->getMakeUniqueFactory<Test>() ) {
				updatePtr( &mTest );
				console() << timer.getSeconds() * 1000.0 << "ms" << endl;
			}
			
		}
	} );
}
#endif

CINDER_APP( CompilerRewriteApp, RendererGl() )
