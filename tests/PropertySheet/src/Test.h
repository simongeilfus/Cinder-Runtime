#pragma once

#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"

class Test {
public:
	Test();

	rt_virtual void draw();
	
protected:

	RT_DECL
};