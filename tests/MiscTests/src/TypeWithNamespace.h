#pragma once

#include "runtime/Virtual.h"
#include "runtime/Factory.h"

namespace foo { namespace bar {

class TypeWithNamespace {
	RT_DECL
public:
	TypeWithNamespace();

	rt_virtual void draw();
};

} } // namespace foo::bar
