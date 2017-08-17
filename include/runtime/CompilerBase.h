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

#include <memory>
#include <functional>

#include "cinder/Exception.h"
#include "cinder/Filesystem.h"
#include "cinder/signals.h"

using ProcessPtr = std::unique_ptr<class Process>;

namespace runtime {

class CompilationResult;

class CompilerBase {
public:
	CompilerBase();
	virtual ~CompilerBase();
	
	//! Compiles and links the file at path. A callback can be specified to get the compilation results.
	virtual void build( const std::string &arguments, const std::function<void(const CompilationResult&)> &onBuildFinish = nullptr ) {}
	
protected:
	virtual std::string		getCLInitCommand() const = 0;
	virtual ci::fs::path	getCompilerPath() const = 0;
	virtual std::string		getCompilerInitArgs() const = 0;

	virtual void parseProcessOutput();
	void initializeProcess();

	ProcessPtr								mProcess;
	ci::signals::ScopedConnection			mProcessOutputConnection;
	bool									mVerbose;
	std::vector<std::string>				mErrors;
	std::vector<std::string>				mWarnings;
};

class CompilationResult {
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
	
	//! Returns the path of the compilation output
	void setOutputPath( const ci::fs::path &path );
	//! Returns the path of the file that has been compiled
	void setPdbFilePath( const ci::fs::path &path );

	CompilationResult();

protected:
	ci::fs::path mOutputPath;
	ci::fs::path mPdbFilePath;
	std::vector<ci::fs::path> mFilePaths;
	std::vector<ci::fs::path> mObjectFilePaths;
	std::vector<std::string> mErrors;
	std::vector<std::string> mWarnings;
};

class CompilerException : public ci::Exception {
public:
	CompilerException( const std::string &message ) : ci::Exception( message ) {}
};

} // namespace runtime

namespace rt = runtime;