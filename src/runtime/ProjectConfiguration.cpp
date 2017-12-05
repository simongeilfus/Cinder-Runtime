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
#include "runtime/ProjectConfiguration.h"
#include "cinder/app/App.h"

#include <sstream>

using namespace std;
using namespace ci;
using namespace ci::app;

namespace runtime {

ProjectConfiguration::ProjectConfiguration( const ci::fs::path &path )
	: mProjectPath( path )
{
	if( mProjectPath.empty() ) {
		fs::path appPath = app::getAppPath();
		const size_t maxDepth = 20;
		size_t depth = 0;
		for( fs::path path = appPath; path.has_parent_path() || ( path == appPath ); path = path.parent_path(), ++depth ) {
			if( depth >= maxDepth || ! mProjectPath.empty() )
				break;

			for( fs::directory_iterator it = fs::directory_iterator( path ), end; it != end; ++it ) {
				if( it->path().extension() == ".vcxproj" ) {
					mProjectPath = it->path();
					break;
				}
			}
		}
		
		if( mProjectPath.empty() ) {
			string msg = "Failed to find the .vcxproj path for this executable.";
			msg += " Searched up " + to_string( maxDepth ) + " levels from app path: " + appPath.string();
			throw ProjectConfigurationException( msg );
		}
	}

	mProjectDir = mProjectPath.parent_path();

#ifdef _WIN64
	mPlatform = "x64";
	mPlatformTarget = "x64";
#else
	mPlatform = "Win32";
	mPlatformTarget = "x86";
#endif

#if defined( _DEBUG )
	mConfiguration = "Debug";
#else
	mConfiguration = "Release";
#endif

#if defined( CINDER_SHARED ) || defined( CINDER_RT_SHARED_BUILD )
	mConfiguration += "_Shared";
#endif
		
#if _MSC_VER == 1900
	mPlatformToolset = "v140";
#elif _MSC_VER >= 1910
	mPlatformToolset = "v141";
#else
	mPlatformToolset = "v120";
#endif
}

string ProjectConfiguration::printToString() const
{
	stringstream str;

	str << "projectPath: " << mProjectPath
		<< "\n\t- configuration: " << mConfiguration << ", platform: " << mPlatform << ", platformTarget: " << mPlatformTarget << ", platformToolset: " << mPlatformToolset;

	return str.str();
}

ProjectConfiguration& ProjectConfiguration::instance()
{
	static ProjectConfiguration config;
	return config;
}

std::string ProjectConfiguration::getConfiguration() const
{ 
	return mConfiguration; 
}
std::string ProjectConfiguration::getPlatform() const
{ 
	return mPlatform; 
}
std::string ProjectConfiguration::getPlatformTarget() const
{ 
	return mPlatformTarget; 
}
std::string ProjectConfiguration::getPlatformToolset() const
{ 
	return mPlatformToolset; 
}
ci::fs::path ProjectConfiguration::getProjectPath() const
{ 
	return mProjectPath; 
}
ci::fs::path ProjectConfiguration::getProjectDir() const
{ 
	return mProjectDir; 
}

void ProjectConfiguration::setConfiguration( const std::string &config )
{ 
	mConfiguration = config; 
}
void ProjectConfiguration::setPlatform( const std::string &platform )
{ 
	mPlatform = platform; 
}
void ProjectConfiguration::setPlatformTarget( const std::string &target )
{ 
	mPlatformTarget = target; 
}
void ProjectConfiguration::setPlatformToolset( const std::string &toolset )
{ 
	mPlatformToolset = toolset; 
}
void ProjectConfiguration::setProjectPath( const ci::fs::path &path )
{ 
	mProjectPath = path; 
}
void ProjectConfiguration::setProjectDir( const ci::fs::path &dir )
{ 
	mProjectDir = dir; 
}

}