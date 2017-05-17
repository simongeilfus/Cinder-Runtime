#pragma once

#include <memory>

// forward declare classes whenever it is possible
// as it will make the class reloading faster
namespace cinder { namespace gl {
	typedef std::shared_ptr<class Batch> BatchRef;
	typedef std::shared_ptr<class TextureFont> TextureFontRef;
} } namespace ci = cinder;

class Test {
public:
	Test();
	virtual void update();
	virtual void draw();
protected:
	ci::gl::TextureFontRef	mFont;
	ci::gl::BatchRef		mBatch;
};