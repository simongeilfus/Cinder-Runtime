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

#include "cinder/Exception.h"
#include "cinder/Filesystem.h"

#include "runtime/Export.h"

namespace runtime {

class CI_RT_API ProjectConfiguration {
public:
	ProjectConfiguration( const ci::fs::path &path = ci::fs::path() );

	static ProjectConfiguration& instance();

	std::string getConfiguration() const;
	std::string getPlatform() const;
	std::string getPlatformTarget() const;
	std::string getPlatformToolset() const;
	ci::fs::path getProjectPath() const;
	ci::fs::path getProjectDir() const;
	
	void setConfiguration( const std::string &config );
	void setPlatform( const std::string &platform );
	void setPlatformTarget( const std::string &target );
	void setPlatformToolset( const std::string &toolset );
	void setProjectPath( const ci::fs::path &path );
	void setProjectDir( const ci::fs::path &dir );

	std::string printToString() const;

protected:
	std::string mConfiguration;
	std::string mPlatform;
	std::string mPlatformTarget;
	std::string mPlatformToolset;
	ci::fs::path mProjectPath;
	ci::fs::path mProjectDir;
};

class CI_RT_API ProjectConfigurationException : public ci::Exception {
public:
	ProjectConfigurationException( const std::string &message ) : ci::Exception( message ) {}
};


} // namespace runtime

namespace rt = runtime;