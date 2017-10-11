#pragma once

#include "cinder/app/App.h"

#if defined( CINDER_SHARED )

#if ! defined( RT_COMPILED )

#include "runtime/ClassWatcher.h"

namespace runtime {

template<typename AppT>
class RtApp : public AppT {
public:
	void rtExecuteLaunch() { executeLaunch(); }
};

template<typename AppT>
void AppMswMain( const ci::app::RendererRef &defaultRenderer, const char *title, const char *sourceFile, const ci::app::AppMsw::SettingsFn &settingsFn = ci::app::AppMsw::SettingsFn(), const rt::CompilerMsvc::BuildSettings &buildSettings = rt::CompilerMsvc::BuildSettings( true ) )
{
	ci::app::Platform::get()->prepareLaunch();

	ci::app::App::Settings settings;
	ci::app::AppMsw::initialize( &settings, defaultRenderer, title ); // AppMsw variant to parse args using msw-specific api

	if( settingsFn )
		settingsFn( &settings );

	if( settings.getShouldQuit() )
		return;

	RtApp<AppT>* app = new RtApp<AppT>;
	app->dispatchAsync( [=]() {
		std::vector<ci::fs::path> sources = { ci::fs::absolute( ci::fs::path( sourceFile ) ) };
		rt::ClassWatcher<AppT>::instance().watch( static_cast<AppT*>( app ), title, 
			sources, buildSettings.getIntermediatePath() / "runtime" / std::string( title ) / "build" / ( std::string( title ) + ".dll" ), rt::Compiler::BuildSettings( buildSettings ).generateFactory( false ) );
		rt::ClassWatcher<AppT>::instance().getModule().getChangedSignal().connect( [=](const Module& module ) {
			app->setup();
		} );
	});

	app->rtExecuteLaunch();

	ci::app::Platform::get()->cleanupLaunch();
	rt::ClassWatcher<AppT>::instance().unwatch( static_cast<AppT*>( app ) );
}
}

#define CINDER_RUNTIME_APP( APP, RENDERER, ... )                                                                        \
int __stdcall WinMain( HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/ )\
{                                                                                                                   \
    cinder::app::RendererRef renderer( new RENDERER );                                                              \
    runtime::AppMswMain<APP>( renderer, #APP, __FILE__, ##__VA_ARGS__ );                                                \
    return 0;                                                                                                       \
}

#else

#define CINDER_RUNTIME_APP( APP, RENDERER, ... )	\
extern "C" __declspec(dllexport) APP* __cdecl rt_new_operator() \
{ \
	return new APP(); \
} \
extern "C" __declspec(dllexport) APP* __cdecl rt_placement_new_operator( APP* address ) \
{ \
	return new (address) APP(); \
} \

#endif

#else
#define CINDER_RUNTIME_APP( APP, RENDERER, ... ) CINDER_APP( APP, RENDERER, ##__VA_ARGS__ )
#endif
