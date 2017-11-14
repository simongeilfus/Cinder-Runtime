#pragma once

#include "cinder/Filesystem.h"
#include "runtime/Export.h"

namespace runtime {

//! Generates the object factory file with a set of extern "C" functions allowing instancing of the class
void CI_RT_API generateClassFactory( const ci::fs::path &outputPath, const std::string &className, const std::string &headerName = "" );

} // namespace runtime

namespace rt = runtime;