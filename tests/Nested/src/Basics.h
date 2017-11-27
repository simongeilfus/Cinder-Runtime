#pragma once

#include "runtime/Virtual.h"
#include "runtime/Factory.h"
#include "Nested.h"

class Basics {
public:
	Basics();
	
	virtual void setup();
	virtual void cleanup();

	// any function that has to be called from
	// outside the class should be marked as virtual
	// rt_virtual should probably be use instead when
	// the function doesn't need to be virtual has it
	// become a no-op when building in Release or Debug.
	rt_virtual void draw();
	
protected:
	std::unique_ptr<Nested> mNested;
	// otherwise RT_DECL is the only thing
	// needed in the class declaration to mark the class
	// as runtime reloadable. Modifying the header is 
	// usually slower than modifying the cpp file.
	RT_DECL
};