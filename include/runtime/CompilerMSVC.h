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
	
	//! Describes the list of Options and arguments available when building a file
	class BuildSettings {
	public:
		//! Adds the default build settings
		BuildSettings& default();

		//! Adds an extra include folder to the compiler BuildSettings
		BuildSettings& include( const ci::fs::path &path );
		//! Adds an extra include folder to the compiler BuildSettings
		BuildSettings& libraryPath( const ci::fs::path &path );
		//! Adds an extra include folder to the compiler BuildSettings
		BuildSettings& library( const std::string &library );
		
		//! Adds a preprocessor definition to the compiler BuildSettings
		BuildSettings& define( const std::string &definition );

		//! Specifies the path to the precompiled header.
		BuildSettings& precompiledHeader( const ci::fs::path &path, bool create );
		//! Adds a forced include as the first lined of the compiled file (If you use multiple /FI options, files are included in the order they are processed by CL.)
		BuildSettings& forceInclude( const std::string &filename );
		//! Specifies an additional file to be compiled (and linked).
		BuildSettings& additionalSource( const ci::fs::path &cppFile );
		//! Specifies additional files to be compiled (and linked).
		BuildSettings& additionalSources( const std::vector<ci::fs::path> &cppFiles );
		
		//! Specifies an object (.obj) file name or directory to be used instead of the default.
		BuildSettings& objectFile( const ci::fs::path &path );
		//! Specifies a file name for the program database (PDB) file created by /Z7, /Zi, /ZI (Debug Information Format).
		BuildSettings& programDatabase( const ci::fs::path &path );

		//! Sets the output path
		BuildSettings& outputPath( const ci::fs::path &path );
		
		//! Adds an obj files to be linked
		BuildSettings& linkObj( const ci::fs::path &path );
		//! Adds the app's generated .obj files to be linked. Default to true
		BuildSettings& linkAppObjs( bool link );
		
		//! Generates a class Factory source. Default to true
		BuildSettings& generateFactory( bool generate );
		
		//! Adds an additional compiler option
		BuildSettings& compilerOption( const std::string &option );
		//! Adds an additional linker option
		BuildSettings& linkerOption( const std::string &option );

		//! Enables verbose mode. Disabled by default.
		BuildSettings& verbose( bool enabled = true );

		BuildSettings();
	protected:
		friend class CompilerMsvc;
		bool mVerbose;
		bool mLinkAppObjs;
		bool mGenerateFactory;
		bool mGeneratePch;
		ci::fs::path mPrecompiledHeader;
		ci::fs::path mOutputPath;
		ci::fs::path mObjectFilePath;
		ci::fs::path mPdbPath;
		std::vector<ci::fs::path> mIncludes;
		std::vector<ci::fs::path> mLibraryPaths;
		std::vector<ci::fs::path> mAdditionalSources;
		std::vector<std::string> mLibraries;
		std::vector<std::string> mPpDefinitions;
		std::vector<std::string> mForcedIncludes;
		std::vector<std::string> mCompilerOptions;
		std::vector<std::string> mLinkerOptions;
		std::vector<ci::fs::path> mObjPaths;
	};
	
	void build( const std::string &arguments, const std::function<void(const CompilationResult&)> &onBuildFinish = nullptr ) override;
	void build( const ci::fs::path &sourcePath, const BuildSettings &settings, const std::function<void(const CompilationResult&)> &onBuildFinish = nullptr );
	void build( const std::vector<ci::fs::path> &sourcesPaths, const BuildSettings &settings, const std::function<void(const CompilationResult&)> &onBuildFinish = nullptr );
	
protected:
	std::string generateCompilerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const;
	std::string generateLinkerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const;
	std::string generateBuildCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const;

	void parseProcessOutput() override;

	std::string		getCLInitCommand() const override;
	ci::fs::path	getCompilerPath() const override;
	std::string		getCompilerInitArgs() const override;

	using Build = std::tuple<CompilationResult,std::function<void(const CompilationResult&)>,std::chrono::steady_clock::time_point>;
	using BuildMap = std::map<ci::fs::path,Build>;

	BuildMap mBuilds;
};

} // namespace runtime

namespace rt = runtime;