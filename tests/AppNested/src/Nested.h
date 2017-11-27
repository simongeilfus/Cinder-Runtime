#pragma once

#include "runtime/Virtual.h"
#include "runtime/Factory.h"

class Nested { 
	RT_DECL
public:
	Nested();
	
	rt_virtual void draw();
};