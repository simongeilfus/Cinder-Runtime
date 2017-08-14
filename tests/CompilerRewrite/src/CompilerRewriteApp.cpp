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

	unique_ptr<Test> mTest;
	Font mFontLarge;
	rt::CompilerPtr mCompiler;
	rt::ModuleRef mModule;
};

void CompilerRewriteApp::setup()
{
	mFontLarge = Font( "Arial", 35 );
	mCompiler = make_unique<rt::Compiler>();
	mTest = make_unique<Test>();
	
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
	if( mModule ) {
		mModule->unlockHandle();
	}

	auto settings = rt::Compiler::BuildSettings()
		.default() // default cinder project settings
		.include( "../../../include" ); // cinder-runtime include folder
		
	// initiate the build
	mCompiler->build( CI_RT_PROJECT_ROOT / "src/Test.cpp", settings, [=]( const rt::CompilationResult &result ) {
		auto dllPath = CI_RT_INTERMEDIATE_DIR / "runtime/Test/Test.dll";
		if( fs::exists( dllPath ) ) {
			
			//mTest.reset();

			if( ! mModule ) {
				mModule = make_shared<rt::Module>( CI_RT_INTERMEDIATE_DIR / "runtime/Test/Test.dll" );
			}
			else {
				mModule->updateHandle();
			}


			if( auto makeUnique = mModule->getMakeUniqueFactory<Test>() ) {
				std::unique_ptr<Test> newPtr;
				makeUnique( &newPtr );
				if( newPtr ) {
					console() << timer.getSeconds() * 1000.0 << "ms" << endl;
					console() << newPtr->getString() << endl;
					mTest = std::move( newPtr );
				}
			}
			
		}
	} );
}

CINDER_APP( CompilerRewriteApp, RendererGl() )
