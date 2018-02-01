/*
 Copyright (c) 2017, Simon Geilfus
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "runtime/BuildOutput.h"

namespace runtime {

BuildOutput::BuildOutput()
: mTimePoint( std::chrono::system_clock::now() )
{
}

ci::fs::path BuildOutput::getOutputPath() const
{
	return mOutputPath;
}
const std::vector<ci::fs::path>& BuildOutput::getFilePaths() const
{
	return mFilePaths;
}
std::vector<ci::fs::path>& BuildOutput::getFilePaths()
{
	return mFilePaths;
}
const std::vector<ci::fs::path>& BuildOutput::getObjectFilePaths() const
{
	return mObjectFilePaths;
}
std::vector<ci::fs::path>& BuildOutput::getObjectFilePaths()
{
	return mObjectFilePaths;
}
ci::fs::path BuildOutput::getPdbFilePath() const
{
	return mPdbFilePath;
}
bool BuildOutput::hasErrors() const
{
	return ! mErrors.empty();
}
const std::vector<std::string>& BuildOutput::getErrors() const
{
	return mErrors;
}
std::vector<std::string>& BuildOutput::getErrors()
{
	return mErrors;
}
const std::vector<std::string>& BuildOutput::getWarnings() const
{
	return mWarnings;
}
std::vector<std::string>& BuildOutput::getWarnings()
{
	return mWarnings;
}

const BuildSettings& BuildOutput::getBuildSettings() const
{
	return mBuildSettings;
}
std::chrono::system_clock::time_point BuildOutput::getTimePoint() const
{
	return mTimePoint;
}

void BuildOutput::setOutputPath( const ci::fs::path &path )
{
	mOutputPath = path;
}
void BuildOutput::setPdbFilePath( const ci::fs::path &path )
{
	mPdbFilePath = path;
}
void BuildOutput::setBuildSettings( const BuildSettings &settings )
{
	mBuildSettings = settings;
}

} // namespace runtime