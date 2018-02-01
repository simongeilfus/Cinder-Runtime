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

#include <map>
#include <vector>
#include <functional>

#include "runtime/Export.h"
#include "runtime/BuildStep.h"
#include "cinder/Filesystem.h"

namespace runtime {

//! Describes the list of Options and arguments available when building a file
class CI_RT_API BuildSettings {
public:
	BuildSettings();
	
#if defined( CINDER_MSW )
	//! Specifies a vcxproj to be parsed for compiler and linker settings. Will use the default project file if null.
	BuildSettings& vcxproj( const ci::fs::path &path = ci::fs::path() );
	//! Specifies a vcxproj to be parsed for compiler settings. Will use the default project file if null.
	BuildSettings& vcxprojCpp( const ci::fs::path &path = ci::fs::path() );
	//! Specifies a vcxproj to be parsed for linker settings. Will use the default project file if null.
	BuildSettings& vcxprojLinker( const ci::fs::path &path = ci::fs::path() );
#endif
	
	//! Adds an extra include folder to the compiler BuildSettings
	BuildSettings& include( const ci::fs::path &path );
	//! Adds an a library search path to the linker BuildSettings
	BuildSettings& libraryPath( const ci::fs::path &path );
	//! Adds a library to the linker BuildSettings
	BuildSettings& library( const std::string &library );
		
	//! Adds a preprocessor definition to the compiler BuildSettings
	BuildSettings& define( const std::string &definition );
		
	//! Specifies whether a precompiled header need to be used.
	BuildSettings& usePrecompiledHeader( bool use = true /*const ci::fs::path &path*/ );
	//! Specifies whether a precompiled header need to be created.
	BuildSettings& createPrecompiledHeader( bool create = true /*const ci::fs::path &path*/ );
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
	//! Specifies an alternate location for the Program Database (.pdb) file in a compiled binary file. Normally, the linker records the location of the .pdb file in the binaries that it produces. You can use this option to provide a different path and file name for the .pdb file. The information provided with /PDBALTPATH does not change the location or name of the actual .pdb file; it changes the information that the linker writes in the binary file. This enables you to provide a path that is independent of the file structure of the build computer.
	BuildSettings& programDatabaseAltPath( const ci::fs::path &path );
		
	//! Sets the output directory path
	BuildSettings& outputPath( const ci::fs::path &path );
	//! Sets the intermediate directory path
	BuildSettings& intermediatePath( const ci::fs::path &path );
		
	//! Specifies the build configuration (Debug_Shared, Release_Shared, Release, Debug, etc...)
	BuildSettings& configuration( const std::string &option );
	//! Specifies the target platform (Win32 or x64)
	BuildSettings& platform( const std::string &option );
	//! Specifies the target platform toolset (v120, v140, v141, etc..)
	BuildSettings& platformToolset( const std::string &option );
				
	//! Specifies the name of the module (.dll). Also used for path generation.
	BuildSettings& moduleName( const std::string &name );

	//! Adds an obj files to be linked
	BuildSettings& linkObj( const ci::fs::path &path );
	//! Specifies a module definition file to be used by the linker
	BuildSettings& moduleDef( const ci::fs::path &path );

	//! Adds a custom operation to be executed before the build
	BuildSettings& preBuildStep( const BuildStepRef &customStep );
	//! Adds a custom operation to be executed after the build
	BuildSettings& postBuildStep( const BuildStepRef &customStep );
		
	//! Adds an additional compiler option
	BuildSettings& compilerOption( const std::string &option );
	//! Adds an additional linker option
	BuildSettings& linkerOption( const std::string &option );
	//! Adds a user macro that will be string replaced in other settings.
	BuildSettings& userMacro( const std::string &name, const std::string &value );

	//! Enables verbose mode. Disabled by default.
	BuildSettings& verbose( bool enabled = true );

	const ci::fs::path& 	getPrecompiledHeader() const { return mPrecompiledHeader; }
	const ci::fs::path& 	getOutputPath() const { return mOutputPath; }
	const ci::fs::path& 	getIntermediatePath() const { return mIntermediatePath; }
	const ci::fs::path& 	getObjectFilePath() const { return mObjectFilePath; }
	const ci::fs::path& 	getPdbPath() const { return mPdbPath; }
	const ci::fs::path& 	getPdbAltPath() const { return mPdbAltPath; }
	const std::string&		getConfiguration() const { return mConfiguration; }
	const std::string&		getPlatform() const { return mPlatform; }
	const std::string&		getPlatformToolset() const { return mPlatformToolset; }
	const std::string&		getModuleName() const { return mModuleName; }

	const std::vector<ci::fs::path>& 	getIncludes() const { return mIncludes; }
	const std::vector<ci::fs::path>& 	getLibraryPaths() const { return mLibraryPaths; }
	const std::vector<ci::fs::path>& 	getAdditionalSources() const { return mAdditionalSources; }
	const std::vector<std::string>& 	getLibraries() const { return mLibraries; }
	const std::vector<std::string>& 	getPpDefinitions() const { return mPpDefinitions; }
	const std::vector<std::string>& 	getForcedIncludes() const { return mForcedIncludes; }
	const std::vector<std::string>& 	getCompilerOptions() const { return mCompilerOptions; }
	const std::vector<std::string>& 	getLinkerOptions() const { return mLinkerOptions; }
	const std::vector<ci::fs::path>& 	getObjPaths() const { return mObjPaths; }

	const std::map<std::string, std::string>&	getUserMacros() const	{ return mUserMacros; };

	bool isVerboseEnabled() const	{ return mVerbose; }

	//! Method meant for debugging purposes to write a pretty string of all settings
	std::string printToString() const;

protected:
	friend class CompilerMsvc;
	bool mVerbose;
	bool mCreatePch;
	bool mUsePch;
	ci::fs::path mPrecompiledHeader;
	ci::fs::path mOutputPath;
	ci::fs::path mIntermediatePath;
	ci::fs::path mObjectFilePath;
	ci::fs::path mPdbPath;
	ci::fs::path mPdbAltPath;
	ci::fs::path mModuleDefPath;
	std::string	mConfiguration;
	std::string	mPlatform;
	std::string	mPlatformToolset;
	std::string mModuleName;
	std::vector<ci::fs::path> mIncludes;
	std::vector<ci::fs::path> mLibraryPaths;
	std::vector<ci::fs::path> mAdditionalSources;
	std::vector<std::string> mLibraries;
	std::vector<std::string> mPpDefinitions;
	std::vector<std::string> mForcedIncludes;
	std::vector<std::string> mCompilerOptions;
	std::vector<std::string> mLinkerOptions;
	std::vector<ci::fs::path> mObjPaths;
	std::map<std::string, std::string>	mUserMacros;
	
	std::vector<BuildStepRef> mPreBuildSteps;
	std::vector<BuildStepRef> mPostBuildSteps;
};

} // namespace runtime

namespace rt = runtime;