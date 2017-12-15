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

#include <vector>
#include <string>

#include "runtime/Export.h"
#include "cinder/Filesystem.h"

namespace runtime {

class CI_RT_API BuildSettings;
class CI_RT_API BuildOutput;

using BuildStepRef = std::shared_ptr<class BuildStep>;

//! Represents a custom operation executed before or after a build
class CI_RT_API BuildStep {
public:
	virtual ~BuildStep();
	virtual void execute( BuildSettings* settings, BuildOutput* output = nullptr ) const = 0;
};

//! BuildStep used to generate the factory source
class CI_RT_API CodeGeneration : public BuildStep {
public:
	class Options {
	public:
		//! Exposes the className class new operator
		Options& newOperator( const std::string &className );
		//! Exposes the className class placement new operator
		Options& placementNewOperator( const std::string &className );
		//! Adds an include at the top of the source
		Options& include( const std::string &filename );

	protected:
		friend class CodeGeneration;
		std::vector<std::string> mNewOperators;
		std::vector<std::string> mPlacementNewOperators;
		std::vector<std::string> mIncludes;
	};

	CodeGeneration( const Options &options );
	void execute( BuildSettings* settings, BuildOutput* output = nullptr ) const override;
protected:
	Options mOptions;
};

//! BuildStep used to parse source, extract includes and generate the source for a precompiled header
class CI_RT_API PrecompiledHeader : public BuildStep {
public:
	class Options {
	public:
		//! Extracts include from source at path
		Options& parseSource( const ci::fs::path &path );
		//! Adds an include to the list of precompiled headers
		Options& include( const std::string &filename, bool angleBrackets = false );
		//! Adds an include to be ignored by the list of precompiled headers
		Options& ignore( const std::string &filename, bool angleBrackets = false );
	protected:
		friend class PrecompiledHeader;
		mutable std::vector<std::string> mIncludes;
		std::vector<std::string> mIgnoredIncludes;
	};
	
	PrecompiledHeader( const Options &options );
	void execute( BuildSettings* settings, BuildOutput* output = nullptr ) const override;
protected:
	Options mOptions;
};

class CI_RT_API ModuleDefinition : public BuildStep {
public:
	class Options {
	public:
		//! 
		Options& exportSymbol( const std::string &symbol );
		//! 
		Options& exportVftable( const std::string &className );
	protected:
		friend class ModuleDefinition;
		std::vector<std::string> mExportSymbols;
	};
	
	//! Returns the compiler-decorated symbol of typeName's vftable.
	static std::string getVftableSymbol( const std::string &typeName );
	
	ModuleDefinition( const Options &options );
	void execute( BuildSettings* settings, BuildOutput* output = nullptr ) const override;
protected:
	Options mOptions;
};

class CI_RT_API LinkAppObjs : public BuildStep {
public:
	void execute( BuildSettings* settings, BuildOutput* output = nullptr ) const override;
protected:
};

class CI_RT_API CopyBuildOutput : public BuildStep {
public:
	void execute( BuildSettings* settings, BuildOutput* output = nullptr ) const override;
};

//class CI_RT_API CleanupBuildFolder : public BuildStep {
//public:
//	void execute( BuildSettings* settings ) const override;
//protected:
//};

//class CI_RT_API CleanupBuildFolder : public BuildStep {
//public:
//	void execute( BuildSettings* settings ) const override;
//protected:
//};

} // namespace runtime

namespace rt = runtime;