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

#include "runtime/Module.h"
#include "cinder/Log.h"
#include <iostream>
#include <fstream>
#include <sstream>

#if ! defined( WIN32_LEAN_AND_MEAN )
	#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

using namespace std;
using namespace ci;

namespace runtime {

Module::Module( const ci::fs::path &path )
: mPath( path ), mName( path.stem().string() )
#if defined( CINDER_MSW )
, mHandle( nullptr )
#endif
{
	if( fs::exists( path ) ) {
#if defined( CINDER_MSW )
		mHandle = LoadLibrary( mPath.wstring().c_str() );
#else
#endif
	}
}

Module::~Module()
{
	// release the library
	if( mHandle ) {
#if defined( CINDER_MSW )
		FreeLibrary( static_cast<HINSTANCE>( mHandle ) );
#endif
	}

	// if the module has been updated there's propbably a temp
	// file that needs to be removed
	if( fs::exists( mTempPath ) ) {
		try {
			fs::remove( mTempPath );
		}
		catch( const fs::filesystem_error & ) {
		}
	}
}

void Module::updateHandle( const ci::fs::path &path )
{
	if( ! path.empty() ) {
		mPath = path;
		mName = mPath.stem().string();
	}

	if( fs::exists( mPath ) ) {
		if( mHandle != nullptr ) {
			FreeLibrary( static_cast<HINSTANCE>( mHandle ) );
		}
#if defined( CINDER_MSW )
		mHandle = LoadLibrary( mPath.wstring().c_str() );
#endif
	}
}

void Module::unlockHandle()
{
	if( fs::exists( mPath ) ) {
		// if the old temp file is still there delete it
		if( fs::exists( mTempPath ) ) {
			try {
				fs::remove( mTempPath );
			}
			catch( const fs::filesystem_error & ) {}
		}

		// rename the soon to be old module to a temp name
		mTempPath = mPath.parent_path() / ( mPath.stem().string() + "_old" + mPath.extension().string() );
		try {
			fs::rename( mPath, mTempPath );
		}
		catch( const fs::filesystem_error & ) {}
	}
}

Module::Handle Module::getHandle() const
{
	return mHandle;
}

ci::fs::path Module::getPath() const
{
	return mPath;
}

ci::fs::path Module::getTempPath() const
{
	return mTempPath;
}
std::string Module::getName() const
{
	return mName;
}

bool Module::isValid() const
{
	return mHandle != nullptr;
}

void* Module::getSymbolAddress( const std::string &symbol ) const
{
	return (void*) GetProcAddress( static_cast<HMODULE>( mHandle ), symbol.c_str() );
}

ci::signals::Signal<void( const Module& )>& Module::getCleanupSignal()
{
	return mCleanupSignal;
}

ci::signals::Signal<void( const Module& )>& Module::getChangedSignal()
{
	return mChangedSignal;
}

} // namespace runtime