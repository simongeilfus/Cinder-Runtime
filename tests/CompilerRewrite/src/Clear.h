#pragma once

#include "cinder/gl/wrapper.h"
#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"

class Clear {
public:
	rt_virtual void clear()
	{
		ci::gl::clear( ci::ColorA( 0, 0, 0, 1 ) );
	}

	RT_WATCH_CLASS_INLINE( Clear );
};