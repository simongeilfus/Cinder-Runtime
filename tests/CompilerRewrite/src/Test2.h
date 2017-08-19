#pragma once

#include <string>
#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"

class Test2 {
public:
	Test2();
	rt_virtual std::string getString();

protected:
	int mCount;
	RT_WATCH_HEADER
};