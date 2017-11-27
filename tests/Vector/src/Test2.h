#pragma once

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "runtime/Virtual.h"
#include "runtime/Factory.h"

class Test2 {
public:
	Test2();
	rt_virtual void draw();
protected:
	ci::vec2 mPosition;
	ci::ColorA mColor;
	float mSize;
	RT_DECL;
};