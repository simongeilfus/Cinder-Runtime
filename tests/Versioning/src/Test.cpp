#include "Test.h"

// when not opting-out from the runtime compiler
// precompiled header options, all the header will
// be added to a precompiled header to make future
// rebuild (much) faster. This makes making changes 
// to include the first or the first compilation longer
#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/wrapper.h"

// see comment in ImGuiApp.cpp about Cinder-ImGui
#include "CinderImGui.h"

using namespace std;
using namespace ci;
using namespace ci::app;

Test::Test()
{
}

void Test::draw()
{
	//ui::ScopedWindow window( "Test" );
	static ColorA background = ColorA( 1.0f, 0.0f, 0.2925f, 1.0f );
	
	//ui::ColorEdit4( "Background", &background[0] );

	gl::clear( background );


	ui::ScopedWindow window2( "Versioning" );
	auto now = chrono::system_clock::now();
	if( auto type = rt::Factory::instance().getType<Test>() ) {
		if( ui::CollapsingHeader( type->getName().c_str(), ImGuiTreeNodeFlags_DefaultOpen ) ) {
			ui::Indent( 20 );
			auto versions = type->getVersions();
			rt::Factory::Type::Version* version = nullptr;
			ui::Columns( 3, nullptr, false );
			for( auto it = versions.rbegin(); it != versions.rend(); ++it ) {
				auto &v = *it;
				auto timePoint = v.getTimePoint();
				auto duration = now - timePoint;
				double time = chrono::duration_cast<chrono::seconds>( duration ).count();
				string timeString = to_string( (int) time ) + " s";
				if( time > 59 ) {
					time /= 60;
					timeString = to_string( (int) time ) + " m";
					if( time > 59 ) {
						time /= 60;
						timeString = to_string( (int) time ) + " h";

						if( time > 23 ) {
							time /= 24;
							timeString = to_string( (float) time ) + " d";
						}
					}
				}
				ui::Text( v.getPath().stem().string().c_str() );
				ui::NextColumn();
				ui::Text( timeString.c_str() );
				if( ui::IsItemHovered() ) {
					ui::BeginTooltip();
					std::time_t t = chrono::system_clock::to_time_t( v.getTimePoint() );
					ui::Text( std::ctime(&t) );
					ui::EndTooltip();
				}
				ui::NextColumn();
				if( ui::Button( "Load" ) ) {
					version = &v;
				}
				ui::SameLine();
				if( ui::Button( "Rollback" ) ) {
				}

				ui::NextColumn();
			}
			ui::Columns( 1 );
			if( version ) {
				rt::Factory::instance().loadTypeVersion( type->getTypeIndex(), *version );
			}

			ui::Unindent( 20 );
		}
	}
}

// RT_IMPL has to be added to mark the class
// as runtime reloadable. 
RT_IMPL( Test );