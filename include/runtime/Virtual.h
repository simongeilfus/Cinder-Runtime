#pragma once

namespace runtime {

#if defined( CINDER_SHARED )
#define rt_virtual virtual
#else
#define rt_virtual
#endif

} // namespace runtime

namespace rt = runtime;