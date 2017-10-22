#pragma once

#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"

namespace foo { namespace bar {

class TypeWithNamespace {
	RT_WATCH_HEADER
public:
	TypeWithNamespace();

	rt_virtual void draw();
};

} } // namespace foo::bar
