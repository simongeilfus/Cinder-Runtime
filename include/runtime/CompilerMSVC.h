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

#include "runtime/CompilerBase.h"
#include "runtime/BuildSettings.h"

namespace runtime {

using Compiler = class CompilerMsvc; // temp shortcut
using CompilerRef = std::shared_ptr<class CompilerMsvc>; // temp shortcut
using CompilerPtr = std::unique_ptr<class CompilerMsvc>; // temp shortcut
using CompilerMsvcRef = std::shared_ptr<class CompilerMsvc>;
using CompilerMsvcPtr = std::unique_ptr<class CompilerMsvc>;

class CompilerMsvc : public CompilerBase {
public:
	CompilerMsvc();
	~CompilerMsvc();

	static CompilerMsvc& instance();
	
	void build( const std::string &arguments, const std::function<void(const CompilationResult&)> &onBuildFinish = nullptr ) override;
	void build( const ci::fs::path &sourcePath, const BuildSettings &settings, const std::function<void(const CompilationResult&)> &onBuildFinish = nullptr );
	void build( const std::vector<ci::fs::path> &sourcesPaths, const BuildSettings &settings, const std::function<void(const CompilationResult&)> &onBuildFinish = nullptr );

	//! Returns the compiler-decorated symbol of typeName's vtable.
	std::string	getSymbolForVTable( const std::string &typeName ) const;

	//! Method meant for debugging purposes to write a pretty string of all settings
	std::string printToString() const;
	//! This logs Compiler, ProjectConfiguration, and BuildSettings to ci::log
	void debugLog( BuildSettings *settings = nullptr ) const;

protected:
	std::string generateCompilerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const;
	std::string generateLinkerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const;
	std::string generateBuildCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const;

	void parseProcessOutput() override;

	std::string		getCLInitCommand() const override;
	ci::fs::path	getCLInitPath() const override;
	ci::fs::path	getCompilerPath() const override;
	std::string		getCompilerInitArgs() const override;
	
	using Build = std::tuple<CompilationResult,std::function<void(const CompilationResult&)>,std::chrono::steady_clock::time_point>;
	using BuildMap = std::map<ci::fs::path,Build>;

	BuildMap mBuilds;
};

} // namespace runtime

namespace rt = runtime;