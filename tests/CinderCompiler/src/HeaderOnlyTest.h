#pragma once

#include "cinder/app/App.h"

class HeaderOnlyTest {
public:
	HeaderOnlyTest() 
	{
		//ci::app::console() << "HeaderOnlyTest constructed" << std::endl;
	}
	
	virtual void render() const
	{
	}
};