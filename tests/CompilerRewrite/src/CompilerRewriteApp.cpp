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

	shared_ptr<Test> mTest;
	Font mFontLarge, mFontSmall;
	rt::CompilerPtr mCompiler;
	rt::ModuleRef mModule;
};

void CompilerRewriteApp::setup()
{
	mFontSmall = Font( "Arial", 15 );
	mFontLarge = Font( "Arial", 35 );
	mCompiler = make_unique<rt::Compiler>();
	mTest = make_unique<Test>();
	
	FileWatcher::instance().watch( { CI_RT_PROJECT_ROOT / "src/Test.h", CI_RT_PROJECT_ROOT / "src/Test.cpp" }, [this]( const WatchEvent &event ) { buildTestCpp(); } );
}

void CompilerRewriteApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	
	gl::drawStringCentered( mTest->getString(), getWindowCenter(), ColorA::white(), mFontLarge );
	//gl::drawStringCentered( CI_RT_PROJECT_PATH.string(), getWindowCenter() + vec2( 0,30 ), ColorA::white(), mFontSmall );
}

void CompilerRewriteApp::buildTestCpp()
{
	Timer timer( true );
	if( mModule ) {
		mModule->unlockHandle();
	}

	auto settings = rt::Compiler::BuildSettings().default()
		.include( "../../../include" )
		//.compilerOption( "/O2" )
		//.compilerOption( "/MP" )
		;
	mCompiler->build( CI_RT_PROJECT_ROOT / "src/Test.cpp", settings, [=]( const rt::CompilationResult &result ) {
		if( ! mModule ) {
			mModule = make_shared<rt::Module>( CI_RT_INTERMEDIATE_DIR / "runtime/Test/Test.dll" );
		}
		else {
			mModule->updateHandle();
		}

		if( auto handle = mModule->getHandle() ) {
			using FactoryPtr = void (__cdecl*)(std::shared_ptr<Test>*);
			if( auto factoryMakeShared = (FactoryPtr) GetProcAddress( static_cast<HMODULE>( handle ), "rt_make_shared" ) ) {
				std::shared_ptr<Test> newPtr;
				factoryMakeShared( &newPtr );
				if( newPtr ) {
					console() << timer.getSeconds() * 1000.0 << "ms" << endl;
					console() << newPtr->getString() << endl;
				}
			}
		}
	} );
	/*mCompiler->build( command, []( const rt::CompilationResult &result ) {
		app::console() << "Done!" << endl;
	} );*/


	//Mscv::Options().includePath( ".." ).libraryPath( "../.." ).source( sourceA ).source( sourceB, { sourceA, sourceC } ).additionalOptions( Msvc::getDefaultFlags() );

	

	//app::console() << ( "../../../../../lib/msw" / fs::path( CI_RT_PLATFORM ) / fs::path( CI_RT_CONFIGURATION ) / fs::path( CI_RT_PLATFORM_TOOLSET ) ).generic_string() << endl;
}

CINDER_APP( CompilerRewriteApp, RendererGl() )
