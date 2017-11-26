/*
 Copyright (c) 2017, Simon Geilfus
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "cinder/app/App.h"

#if defined( CINDER_SHARED )

#if ! defined( RT_COMPILED )

#include "runtime/Factory.h"

namespace runtime {

template<typename AppT>
class RtApp : public AppT {
public:
	void rtExecuteLaunch() { executeLaunch(); }
};

template<typename AppT>
void AppMswMain( const ci::app::RendererRef &defaultRenderer, const char *title, const char *sourceFile, const ci::app::AppMsw::SettingsFn &settingsFn = ci::app::AppMsw::SettingsFn(), const rt::BuildSettings &buildSettings = rt::BuildSettings().vcxproj() )
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
		rt::Factory::instance().watch<AppT>( std::type_index(typeid(AppT)), static_cast<AppT*>( app ), title, sources, buildSettings.getIntermediatePath() / "runtime" / std::string( title ) / "build" / ( std::string( title ) + ".dll" ), buildSettings, rt::Factory::TypeFormat().classFactory( false ) );
	});

	app->rtExecuteLaunch();

	ci::app::Platform::get()->cleanupLaunch();
	rt::Factory::instance().unwatch( std::type_index(typeid(AppT)), static_cast<AppT*>( app ) );
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
