#include "TypeWithNamespace.h"

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

RT_WATCH_IMPL( foo::bar::TypeWithNamespace,  rt::CompilerMsvc::BuildSettings( true ).verbose( true ) );

using namespace std;
using namespace ci;

namespace foo { namespace bar {

TypeWithNamespace::TypeWithNamespace()
{
}

void TypeWithNamespace::draw()
{
	gl::ScopedColor col( 1, 1, 0 );
	gl::drawSolidCircle( app::getWindowCenter(), 100 );
}

} } // namespace foo::bar
