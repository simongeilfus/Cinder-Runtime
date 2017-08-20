#pragma once

#include <string>
#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"
#include "cinder/gl/Batch.h"

class Test2 {
public:
	Test2();
	rt_virtual void draw();
	rt_virtual std::string getString();

protected:
	int mCount;
	ci::gl::BatchRef mBatch;
	RT_WATCH_HEADER
};