#pragma once

#include <string>
#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"
#include "cinder/gl/Batch.h"
#include "Test.h"

class Test2 {
public:
	Test2();
	rt_virtual void draw();
	rt_virtual std::string getString();

protected:
	std::unique_ptr<Test> mTest;
	int mCount;
	ci::gl::BatchRef mBatch;
	RT_WATCH_HEADER
};