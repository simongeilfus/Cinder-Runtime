#pragma once

#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"

class Basics {
public:
	Basics();
	rt_virtual void draw();

protected:
	RT_WATCH_HEADER;
};