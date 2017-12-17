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
#pragma once

#include "runtime/BuildSettings.h"

namespace runtime {

class CI_RT_API BuildOutput {
public:
	//! Returns the path of the compilation output
	ci::fs::path getOutputPath() const;
	//! Returns the path of the file that has been compiled
	const std::vector<ci::fs::path>&	getFilePaths() const;
	//! Returns the path of the file that has been compiled
	std::vector<ci::fs::path>&			getFilePaths();
	//! Returns the path of the file that has been compiled
	const std::vector<ci::fs::path>&	getObjectFilePaths() const;	
	//! Returns the path of the file that has been compiled
	std::vector<ci::fs::path>&			getObjectFilePaths();	
	//! Returns the path of the file that has been compiled
	ci::fs::path getPdbFilePath() const;
	//! Returns whether the compilation ended with errors
	bool hasErrors() const;
	//! Returns the list of errors 
	const std::vector<std::string>&	getErrors() const;
	//! Returns the list of errors 
	std::vector<std::string>&		getErrors();
	//! Returns the list of warnings
	const std::vector<std::string>& getWarnings() const;
	//! Returns the list of warnings
	std::vector<std::string>&		getWarnings();
	
	//! Returns the settings used to execute that build
	const BuildSettings& getBuildSettings() const;
	//! Returns when this build started
	std::chrono::system_clock::time_point getTimePoint() const;

	//! Sets the path of the compilation output
	void setOutputPath( const ci::fs::path &path );
	//! Sets the path of the file that has been compiled
	void setPdbFilePath( const ci::fs::path &path );
	//! Sets the settings used to execute that build
	void setBuildSettings( const BuildSettings &settings );

	BuildOutput();

protected:
	ci::fs::path mOutputPath;
	ci::fs::path mPdbFilePath;
	std::vector<ci::fs::path> mFilePaths;
	std::vector<ci::fs::path> mObjectFilePaths;
	std::vector<std::string> mErrors;
	std::vector<std::string> mWarnings;
	BuildSettings mBuildSettings;
	std::chrono::system_clock::time_point mTimePoint;
};

} // namespace runtime

namespace rt = runtime;