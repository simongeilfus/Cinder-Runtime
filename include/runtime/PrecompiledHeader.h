#pragma once

#include "cinder/Filesystem.h"
#include "runtime/Export.h"

namespace runtime {

//! Generates the object factory file with a set of extern "C" functions allowing instancing of the class
bool CI_RT_API generatePrecompiledHeader( const ci::fs::path &sourcePath, const ci::fs::path &outputHeader, const ci::fs::path &outputCpp, bool force );

} // namespace runtime

namespace rt = runtime;