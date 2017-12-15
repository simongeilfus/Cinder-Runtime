/*
 Copyright (c) 2016, Simon Geilfus
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

#include "cinder/Filesystem.h"
#include "cinder/Signals.h"

#include "runtime/Export.h"

namespace runtime {

using ModulePtr = std::unique_ptr<class Module>;
using ModuleRef = std::shared_ptr<class Module>;

class CI_RT_API Module : public std::enable_shared_from_this<Module> {
public:
	//! Constructs a new Module object
	Module( const ci::fs::path &path );
	//! Destroys the Module object, release its handles and delete the temporary files
	~Module();

	//! Updates the module with a new handle
	void updateHandle( const ci::fs::path &path = ci::fs::path() );
	//! Changes the disk name of the current module to enable writing a new one 
	void unlockHandle();
	
#if defined( CINDER_MSW )
	// Alias to Windows HINSTANCE
	using Handle = void*;
#else
#endif
	
	//! Returns the current Handle to the module
	Handle getHandle() const;
	//! Returns the current path to the module
	ci::fs::path getPath() const;
	//! Returns the temporary path to the module
	ci::fs::path getTempPath() const;
	//! Returns whether the current Handle is valid
	bool isValid() const;

	void*	getSymbolAddress( const std::string &symbol ) const;
	
	//! Returns the signal used to notify when the Module/Handle is about to be unloaded
	ci::signals::Signal<void(const Module&)>& getCleanupSignal();
	//! Returns the signal used to notify Module/Handle changes
	ci::signals::Signal<void(const Module&)>& getChangedSignal();

protected:
	Handle			mHandle;
	ci::fs::path	mPath, mTempPath;
	
	ci::signals::Signal<void(const Module&)> mChangedSignal;
	ci::signals::Signal<void(const Module&)> mCleanupSignal;
};

} // namespace runtime

namespace rt = runtime;