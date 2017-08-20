#include "Test2.h"
#include "cinder/Area.h"
#include "cinder/gl/Shader.h"

#include <boost/preprocessor/facilities/overload.hpp>

using namespace std;
using namespace ci;

Test2::Test2()
: mCount( 0 )
{
	mCount = 10;
	mTest = make_unique<Test>();
	mBatch = gl::Batch::create( geom::Rect( Rectf( vec2(0), vec2(10) ) ), gl::getStockShader( gl::ShaderDef().color() ) );
}

std::string Test2::getString()
{

	mCount++;
	//return "Hello!";
	return ( "_" + std::to_string( mCount ) + mTest->getString() );
}


void Test2::draw()
{
	gl::translate( vec2(120,220) );
	mBatch->draw();

}

std::string quote( const char* str )
{
	stringstream ss;
	ss << std::quoted( str );
	return ss.str();
}

std::string quote( const std::string &str )
{
	stringstream ss;
	ss << std::quoted( str );
	return ss.str();
}

std::string quote( const ci::fs::path &path )
{
	stringstream ss;
	ss << std::quoted( path.generic_string() );
	return ss.str();
}

RT_WATCH_IMPL( 
	Test2,
	rt::Compiler::BuildSettings().default()
		//.define( "RT_COMPILED" )
	.include( "../../../include" )
	.define( "BLA2" ) 
	.include( "../../../include" )
	.define( quote( "CI_RT_CINDER_PATH=ci::fs::path( " + quote( "../../../../.." ) + " )" ) )
	.define( quote( "CI_RT_PROJECT_ROOT=ci::fs::path( " + quote( CI_RT_PROJECT_ROOT ) + " )" ) )
	.define( quote( "CI_RT_PROJECT_DIR=ci::fs::path( " + quote( CI_RT_PROJECT_DIR ) + " )" ) )
	.define( quote( "CI_RT_PROJECT_PATH=ci::fs::path( " + quote( CI_RT_PROJECT_PATH ) + " )" ) )
	.define( quote( "CI_RT_PLATFORM_TARGET=" + quote( CI_RT_PLATFORM_TARGET ) ) )
	.define( quote( "CI_RT_CONFIGURATION=" + quote( CI_RT_CONFIGURATION ) ) )
	.define( quote( "CI_RT_PLATFORM_TOOLSET=" + quote( CI_RT_PLATFORM_TOOLSET ) ) )
	.define( quote( "CI_RT_INTERMEDIATE_DIR=ci::fs::path( " + quote( CI_RT_INTERMEDIATE_DIR ) + " )" ) )
	.define( quote( "CI_RT_OUTPUT_DIR=ci::fs::path( " + quote( CI_RT_OUTPUT_DIR ) + " )" ) )
)