#pragma once

#include "cinder/Font.h"

class Test {
public:
	Test();
	
	virtual void clear() const;
	virtual void render() const;
	virtual std::string getText() const;

	ci::Font mFont;
};