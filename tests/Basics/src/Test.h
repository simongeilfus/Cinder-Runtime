#pragma once

#include <memory>

// forward declare classes whenever it is possible
// as it will make the class reloading faster
namespace cinder { namespace gl {
	typedef std::shared_ptr<class Batch> BatchRef;
} } namespace ci = cinder;

class Test {
public:
	Test();
	virtual void update();
	virtual void draw();
protected:
	ci::gl::BatchRef mBatch;
};