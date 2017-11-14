/*
 Minimal C++ Process Library
 
 Copyright (c) 2016, Simon Geilfus, All rights reserved.
 
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

#include <string>
#include <memory>
#include <thread>
#include <future>
#include <ostream>
#include <sstream>
#include <queue>

#include "runtime/Export.h"

using ProcessPtr = std::unique_ptr<class Process>;
using ProcessRef = std::shared_ptr<class Process>;

class CI_RT_API Process {
public:
	//! Constructs and initialize a new process in the current directory. Will by default redirect the content of StdOut, StdErr and StdIn.
	Process( const std::string &cmd, bool redirectOutput = true, bool redirectError = true, bool redirectInput = true );
	//! Constructs and initialize a new process in the specified directory. Will by default redirect the content of StdOut, StdErr and StdIn.
	Process( const std::string &cmd, const std::string &path, bool redirectOutput = true, bool redirectError = true, bool redirectInput = true );
	
	//! Waits for the output redirection thread to finish and returns its string 
	std::string	getOutputSync();
	//! Waits for the error redirection thread to finish and returns its string
	std::string	getErrorSync();
	//! Pops and returns the first output available. Returns an empty string if no output is available.
	std::string	getOutputAsync();
	//! Pops and returns the first error available. Returns an empty string if no error is available.
	std::string	getErrorAsync();
	//! Returns whether the a new output from the redirection thread is available
	bool isOutputAvailable();
	//! Returns whether the error from the redirection thread is available
	bool isErrorAvailable();

	//! Waits for the process to terminate and returns its exit code
	int16_t terminate();
	//! Closes the input pipe and read all waiting commands
	void closeInput();

	//! Destructor (Closes the process and threads)
	~Process();
protected:
	
	//! Write to the input pipe
	bool write( const std::string &cmd );
	template <typename T> friend inline Process& operator <<(Process& process, const T &value );
	template <typename T> friend inline ProcessPtr& operator <<(ProcessPtr& process, const T &value );
	friend inline Process& operator <<(Process& process, std::ostream&(*f)(std::ostream&) );
	friend inline ProcessPtr& operator <<(ProcessPtr& process, std::ostream&(*f)(std::ostream&) );

	bool							mProcessRunning;
	bool							mIsOutputQueueEmpty;
	bool							mIsErrorQueueEmpty;
	std::mutex						mOutputQueueMutex;
	std::mutex						mOutputQueueEmptyMutex;
	std::mutex						mErrorQueueMutex;
	std::mutex						mErrorQueueEmptyMutex;
	std::queue<std::string>			mOutputQueue;
	std::queue<std::string>			mErrorQueue;
	std::unique_ptr<std::thread>	mOutputReadThread;
	std::unique_ptr<std::thread>	mErrorReadThread;
	std::future<std::string>		mOutputFuture;
	std::future<std::string>		mErrorFuture;

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	void* mOutputRead;
	void* mErrorRead;
	void* mInputWrite;
	void* mProcess;
#else
#endif
};

template <typename T>
inline Process& operator <<(Process& process, const T &value ) 
{
	std::stringstream ss;
	ss << value;
	process.write( ss.str() );
    return process;
}
inline Process& operator <<(Process& process, std::ostream&(*f)(std::ostream&) ) 
{
    if( f == std::endl<char, std::char_traits<char>> ) {
		process.write( "\n" );
    }
    return process;
}

template <typename T>
inline ProcessPtr& operator <<(ProcessPtr& process, const T &value )
{
	std::stringstream ss;
	ss << value;
	process->write( ss.str() );
    return process;
}

inline ProcessPtr& operator <<(ProcessPtr& process, std::ostream&(*f)(std::ostream&) ) 
{
    if( f == std::endl<char, std::char_traits<char>> ) {
		process->write( "\n" );
    }
    return process;
}

class CI_RT_API ProcessExc : public std::exception {
public:
	ProcessExc() {}
	ProcessExc( const std::string &description ) : mDescription( description ) {}
	virtual ~ProcessExc() throw() {}
	const char* what() const throw() override { return mDescription.c_str(); }
protected:
	std::string mDescription;
};