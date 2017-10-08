#if 1

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/FileWatcher.h"

#include "runtime/CompilerMsvc.h"
#include "runtime/Module.h"
#include "runtime/make_shared.h"

#include "Test.h"
#include "Test2.h"
#include "Clear.h"

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


	unique_ptr<Clear> mClear;
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
	mTest3 = make_shared<Test2>(); // make_shared won't work unfortunately
	mTest4 = new Test2();
	mClear = make_unique<Clear>();
	{
		auto test5 = make_unique<Test2>();
	}
	
#if defined( CINDER_SHARED )
	// init module and watch .h and .cpp
	mModule = make_unique<rt::Module>( CI_RT_INTERMEDIATE_DIR / "runtime/Test/build/Test.dll" );
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
	if( mClear ) {
		mClear->clear();
	}
	
	gl::setMatricesWindow( getWindowSize() );

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
	
	mTest2->draw();
}

#if defined( CINDER_SHARED )
void CompilerRewriteApp::buildTestCpp()
{
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
			/*if( auto updatePtr = mModule->getMakeUniqueFactory<Test>() ) {
				updatePtr( &mTest );
			}*/			
		}
	} );
}
#endif

CINDER_APP( CompilerRewriteApp, RendererGl() )


#else


#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CompilerRewriteApp : public App {
public:
	CompilerRewriteApp() = default;
	CompilerRewriteApp(const CompilerRewriteApp &) {}
	//CompilerRewriteApp(){}
	void draw() override;
	
//#if defined( RT_COMPILED )
	//CompilerRewriteApp( bool dummy ) : App( dummy ) {}
//#endif
};

void CompilerRewriteApp::draw()
{
	
	gl::clear( Color( 0, 0, 0 ) ); 
}

#if  ! defined( RT_COMPILED )

#include "runtime/CompilerMsvc.h"
#include "runtime/Module.h"
#include "cinder/FileWatcher.h"
#include <Windows.h>

template<class T>
void watchApp( T* ptr, const ci::fs::path &source, const ci::fs::path &dllPath, const rt::Compiler::BuildSettings &settings = rt::Compiler::BuildSettings() )
{
	static rt::ModulePtr sModule;
	static T* sInstance = ptr;

	if( ! sModule ) {
		sModule = std::make_unique<rt::Module>( dllPath );
		ci::FileWatcher::instance().watch( source, 
			ci::FileWatcher::Options().callOnWatch( false ),
			[&,source,settings]( const ci::WatchEvent &event ) {
				// unlock the dll-handle before building
				sModule->unlockHandle();
		
				// initiate the build
				rt::CompilerMsvc::instance().build( source, settings, [&,event]( const rt::CompilationResult &result ) {
					// if a new dll exists update the handle
					if( ci::fs::exists( sModule->getPath() ) ) {
						sModule->updateHandle();
						// and try to get a ptr to its make_unique factory function
						void* copyConstruct = (void*) GetProcAddress( static_cast<HMODULE>( sModule->getHandle() ), "rt_copy_construct" );
						if( copyConstruct ) {
						//if( auto makeRaw = sModule->getMakeRawFactory<T>() ) {
							// create a single new instance an use its vtable to override the originals's vtables
							//
							//void* vtableFn = (void*) GetProcAddress( static_cast<HMODULE>( sModule->getHandle() ), "rt_vtable" );
							//void* vtable = ( (void*(__cdecl*)()) vtableFn )();
							//*(void **)sInstance = vtable;
							T* current = static_cast<T*>( ci::app::App::get() );
							T* newPtr = ( (T*(__cdecl*)(T*)) copyConstruct )( current );
							//CompilerRewriteApp bla;
							//auto blabla = new CompilerRewriteApp( bla );
							*(void **)sInstance = *(void**) newPtr;
							::operator delete( newPtr );
						}
			
					}
				} );
			} 
		);
	}
}

namespace cinder { namespace app {
template<>
void AppMsw::main<CompilerRewriteApp>( const RendererRef &defaultRenderer, const char *title, const SettingsFn &settingsFn )
{
	AppBase::prepareLaunch();
	
	Settings settings;
	AppMsw::initialize( &settings, defaultRenderer, title ); // AppMsw variant to parse args using msw-specific api

	if( settingsFn )
		settingsFn( &settings );

	if( settings.getShouldQuit() )
		return;
	
	auto cppPath = ci::fs::absolute( ci::fs::path( __FILE__ ) );
	Settings* settingsPtr = &settings;
	AppMsw *app = static_cast<AppMsw *>( new CompilerRewriteApp );
	app->dispatchAsync( [settingsPtr,app,cppPath]() {
		watchApp( static_cast<CompilerRewriteApp*>( app ), cppPath, CI_RT_INTERMEDIATE_DIR / "runtime" / std::string( "CompilerRewriteApp" ) / ( std::string( "CompilerRewriteApp" ) + ".dll" ), rt::Compiler::BuildSettings().generateFactory( false ).default().define( "RT_COMPILED" ).include( "../../../include" ) );\
	} );
	app->executeLaunch();

	AppBase::cleanupLaunch();
}
}}

CINDER_APP( CompilerRewriteApp, RendererGl() )

#else

extern "C" __declspec(dllexport) CompilerRewriteApp* __cdecl rt_make_raw()
{
	return new CompilerRewriteApp();
}
extern "C" __declspec(dllexport) CompilerRewriteApp* __cdecl rt_copy_construct( CompilerRewriteApp* current )
{
	return new CompilerRewriteApp( *current );
}

extern "C" __declspec(dllexport) void* __cdecl rt_vtable()
{
	CompilerRewriteApp* ptr;
	void* vtable = *(void **) ptr;
	//ci::app::App::get()->dispatchAsync( [ptr]() {
	//	free( ptr );
	//} );
	return vtable;
}

#endif

#endif