#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/FileWatcher.h"

#include "runtime/CompilerMsvc.h"
#include "runtime/Module.h"

#include "Test.h"
#include <Windows.h>

using namespace ci;
using namespace ci::app;
using namespace std;

class CompilerRewriteApp : public App {
public:
	void setup() override;
	void draw() override;
	
	void buildTestCpp();
	
	rt::ModulePtr mModule;
	unique_ptr<Test> mTest;
	Font mFontLarge;
};

void CompilerRewriteApp::setup()
{
	mFontLarge = Font( "Arial", 35 );
	mTest = make_unique<Test>();
	mModule = make_unique<rt::Module>( CI_RT_INTERMEDIATE_DIR / "runtime/Test/Test.dll" );
	
	// watch .h and .cpp
	FileWatcher::instance().watch( { CI_RT_PROJECT_ROOT / "src/Test.h", CI_RT_PROJECT_ROOT / "src/Test.cpp" }, [this]( const WatchEvent &event ) { buildTestCpp(); } );
}

void CompilerRewriteApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	
	if( mTest ) {
		gl::drawStringCentered( mTest->getString(), getWindowCenter(), ColorA::white(), mFontLarge );
	}
}

void CompilerRewriteApp::buildTestCpp()
{
	Timer timer( true );

	// unlock the dll-handle before building
	mModule->unlockHandle();
	
	// prepare build settings
	auto settings = rt::Compiler::BuildSettings()
		.default() // default cinder project settings
		.include( "../../../include" ); // cinder-runtime include folder
		
	// initiate the build
	rt::CompilerMsvc::instance().build( CI_RT_PROJECT_ROOT / "src/Test.cpp", settings, [=]( const rt::CompilationResult &result ) {
		auto dllPath = CI_RT_INTERMEDIATE_DIR / "runtime/Test/Test.dll";
		// if a new dll exists update the handle
		if( fs::exists( dllPath ) ) {
			//mTest.reset();
			mModule->updateHandle();
			// and try to get a ptr to its make_unique factory function
			if( auto makeUnique = mModule->getMakeUniqueFactory<Test>() ) {
				std::unique_ptr<Test> newPtr;
				makeUnique( &newPtr );
				if( newPtr ) {
					console() << timer.getSeconds() * 1000.0 << "ms" << endl;
					mTest = std::move( newPtr );
				}
			}
			
		}
	} );
}

CINDER_APP( CompilerRewriteApp, RendererGl() )
