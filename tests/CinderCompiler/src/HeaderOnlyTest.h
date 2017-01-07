#pragma once

#include "cinder/app/App.h"
#include "cinder/Log.h"

class HeaderOnlyTest {
public:
	HeaderOnlyTest() 
	{
		CI_LOG_I( "bang" );
	}
	
	virtual void render() const
	{
	}
};