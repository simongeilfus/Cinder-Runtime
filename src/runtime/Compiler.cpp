#include "runtime/Compiler.h"

#if ! defined( WIN32_LEAN_AND_MEAN )
	#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <codecvt>
#include <regex>
#include <string>
#include <memory>
#include <thread>
#include <future>
#include <fstream>
#include <ostream>
#include <sstream>
#include <queue>
#include <array>

#include "cinder/Utilities.h"
#include "cinder/app/App.h"

using namespace std;
using namespace ci;

namespace runtime {

Compiler::Options::Options()
: mVerbose( false ), mCreatePrecompiledHeader( false ), mDumpSymbols( true )
{
}
Compiler::Options& Compiler::Options::precompiledHeader( const ci::fs::path &path, bool create )
{
	mPrecompiledHeader = path;
	mCreatePrecompiledHeader = create;
	return *this;
}
Compiler::Options& Compiler::Options::compilerArgs( const std::string &args )
{
	mCompileArgs = args;
	return *this;
}
Compiler::Options& Compiler::Options::linkArgs( const std::string &args )
{
	mLinkArgs = args;
	return *this;
}
Compiler::Options& Compiler::Options::verbose( bool enabled )
{
	mVerbose = enabled;
	return *this;
}
Compiler::Options& Compiler::Options::preprocessorDef( const std::string &definition )
{
	mPpDefinitions.push_back( definition );
	return *this;
}
Compiler::Options& Compiler::Options::include( const std::string &path )
{
	mIncludes.push_back( path );
	return *this;
}
Compiler::Options& Compiler::Options::include( const ci::fs::path &path )
{
	mIncludes.push_back( path.string() );
	return *this;
}
Compiler::Options& Compiler::Options::additionalCompileList( const std::vector<ci::fs::path> &cppFiles )
{
	mCompileList = cppFiles;
	return *this;
}
Compiler::Options& Compiler::Options::additionalLinkList( const std::vector<ci::fs::path> &objFiles )
{
	mLinkList = objFiles;
	return *this;
}

Compiler::Options& Compiler::Options::outputPath( const ci::fs::path &path )
{
	mOutputPath = path;
	return *this;
}

Compiler::Options& Compiler::Options::dumpSymbols( bool dump )
{
	mDumpSymbols = dump;
	return *this;
}

Compiler::Options& Compiler::Options::forceInclude( const std::string &filename )
{
	mForcedIncludes.push_back( filename );
	return *this;
}

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

class Process {
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
	template <typename T> friend inline std::unique_ptr<Process>& operator <<(std::unique_ptr<Process>& process, const T &value );
	friend inline Process& operator <<(Process& process, std::ostream&(*f)(std::ostream&) );
	friend inline std::unique_ptr<Process>& operator <<(std::unique_ptr<Process>& process, std::ostream&(*f)(std::ostream&) );

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
inline std::unique_ptr<Process>& operator <<(std::unique_ptr<Process>& process, const T &value )
{
	std::stringstream ss;
	ss << value;
	process->write( ss.str() );
    return process;
}

inline std::unique_ptr<Process>& operator <<(std::unique_ptr<Process>& process, std::ostream&(*f)(std::ostream&) ) 
{
    if( f == std::endl<char, std::char_traits<char>> ) {
		process->write( "\n" );
    }
    return process;
}

class ProcessExc : public std::exception {
public:
	ProcessExc() {}
	ProcessExc( const std::string &description ) : mDescription( description ) {}
	virtual ~ProcessExc() throw() {}
	const char* what() const throw() override { return mDescription.c_str(); }
protected:
	std::string mDescription;
};


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

namespace {
	//! Managed Handle class. Release handle on destruction
	class ManagedHandle {
	public:
		ManagedHandle();
		~ManagedHandle();
		HANDLE detach();
		operator HANDLE() const { return mHandle; }
		HANDLE* operator&() { return &mHandle; }
	  private:
		HANDLE mHandle;
	};
	
	ManagedHandle::ManagedHandle() 
	: mHandle( INVALID_HANDLE_VALUE ) {}
	ManagedHandle::~ManagedHandle() 
	{
		if( mHandle != INVALID_HANDLE_VALUE ) {
			CloseHandle( mHandle );
		}
	}
	HANDLE ManagedHandle::detach() 
	{
		HANDLE handle = mHandle;
		mHandle = INVALID_HANDLE_VALUE;
		return handle;
	}

} // anonymous namespace
#else
#endif

// https://support.microsoft.com/en-us/kb/190351
// https://msdn.microsoft.com/en-us/library/ms682499.aspx
// 
// https://www.codeproject.com/Articles/5531/Redirecting-an-arbitrary-Console-s-Input-Output
// https://www.codeproject.com/Articles/2996/Universal-Console-Redirector
// Good explanation on Process creation flags
// http://stackoverflow.com/questions/14958276/createprocess-does-not-create-additional-console-windows-under-windows-7/18831029#18831029
// Article on Quote/Espace Cmd
// https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
// random https://aljensencprogramming.wordpress.com/tag/createprocess/
// namedpipes https://msdn.microsoft.com/en-us/library/aa365603(VS.85).aspx
// namedpipes https://www.daniweb.com/programming/software-development/threads/295780/using-named-pipes-with-asynchronous-i-o-redirection-to-winapi

Process::Process( const std::string &cmd, bool redirectOutput, bool redirectError, bool redirectInput ) : Process( cmd, "", redirectOutput, redirectError, redirectInput ) {}
Process::Process( const std::string &cmd, const std::string &path, bool redirectOutput, bool redirectError, bool redirectInput )
: mIsOutputQueueEmpty( true ), mIsErrorQueueEmpty( true )
{

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	mOutputRead = nullptr;
	mErrorRead = nullptr;
	mInputWrite = nullptr;
	ManagedHandle outputRead;
	ManagedHandle outputWrite;
	ManagedHandle errorRead;
	ManagedHandle errorWrite;
	ManagedHandle inputRead;
	ManagedHandle inputWrite;

	// Setup security attributes
	SECURITY_ATTRIBUTES securityAttributes;
	securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttributes.lpSecurityDescriptor = nullptr;
	securityAttributes.bInheritHandle = TRUE;

	// Create the output pipe
	if( redirectOutput ) {
		if( ! CreatePipe( &outputRead, &outputWrite, &securityAttributes, 0 ) ||
			! SetHandleInformation( outputRead, HANDLE_FLAG_INHERIT, 0 ) ) {
			throw ProcessExc( "Failed Creating StdOut Pipe" );
		}
    }
	
	// Create the error pipe
	if( redirectError ) {
		if( ! CreatePipe( &errorRead, &errorWrite, &securityAttributes, 0 ) ||
			! SetHandleInformation( errorRead, HANDLE_FLAG_INHERIT, 0 ) ) {
			throw ProcessExc( "Failed Creating StdErr Pipe" );
		}
	}

	// Create the input pipe
	if( redirectInput ) {
		if( ! CreatePipe( &inputRead, &inputWrite, &securityAttributes, 0 ) ||
			! SetHandleInformation( inputWrite, HANDLE_FLAG_INHERIT, 0 ) 
			) {
			throw ProcessExc( "Failed Creating StdIn Pipe" );
		}
	}

	// Setup the startup info struct
	STARTUPINFOW startupInfo;
	ZeroMemory( &startupInfo, sizeof(startupInfo) );
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.wShowWindow = SW_HIDE;
	if( redirectOutput ) {
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdOutput = outputWrite;
	}
	if( redirectError ) {
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdError = errorWrite;
	}
	if( redirectInput ) {
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdInput = inputRead;
	}
	
	// Initialize the new process
	PROCESS_INFORMATION processInfo;
	ZeroMemory( &processInfo, sizeof(processInfo) );
	DWORD creationFlags = CREATE_UNICODE_ENVIRONMENT;//| CREATE_NEW_CONSOLE; //0;
	if( ! CreateProcessW( nullptr,							// Application Name
						 cmd.empty() ?						// Command Line
						 nullptr : (LPWSTR) std::wstring( cmd.begin(), cmd.end() ).c_str(),	
						 nullptr,							// Process Attributes (If NULL, the handle cannot be inherited.)
						 nullptr,							// Thread Attributes (If NULL, the handle cannot be inherited.)
						 TRUE,								// Inherit Handles
						 creationFlags,						// Creation Flags
						 nullptr,							// Environment (If NULL, the new process uses the environment of the calling process.)
						 path.empty() ?						// Current Path (If NULL, the new process will have the same current drive and directory as the calling process.)
						 nullptr : (LPWSTR) std::wstring( path.begin(), path.end() ).c_str(),	
						 &startupInfo,						// Startup info
						 &processInfo						// Process info
	) ) {	
		// Close ProcessInfo process and thread handles
		CloseHandle( processInfo.hProcess );
		CloseHandle( processInfo.hThread );
		mProcessRunning = false;
		throw ProcessExc( "Failed Creating Process" );
	}
	else {
		// Close ProcessInfo thread handle
		CloseHandle( processInfo.hThread );
		// And keep a reference to its process handle
		mProcess = processInfo.hProcess;
		mProcessRunning = true;
	}
	
	// Detach the temporary output handles and keep the read ends
	if( redirectOutput ) {
		mOutputRead = outputRead.detach();
	}
	if( redirectError ) {
		mErrorRead = errorRead.detach();
	}
	if( redirectInput ) {
		mInputWrite = inputWrite.detach();
	}

	// Create the async output read thread
	if( redirectOutput ) {
		mOutputReadThread = std::make_unique<std::thread>( [this]() {
			std::promise<std::string> outputPromise;
			mOutputFuture = outputPromise.get_future();
			std::string outputStr;
			DWORD readBytes;
			std::array<char, 65536> buffer;
			while( ReadFile( mOutputRead, buffer.data(), static_cast<DWORD>(65536), &readBytes, nullptr ) && readBytes != 0 ) {
				// copy the buffer into a string
				std::string output( buffer.begin(), buffer.begin() + readBytes );
				// add the string to the sync output
				outputStr += output;
				// push the string to the async queue
				std::lock_guard<std::mutex> queueLock( mOutputQueueMutex );
				bool wasEmpty = mOutputQueue.empty();
				mOutputQueue.push( output );
				if( wasEmpty ) {
					std::lock_guard<std::mutex> queueEmptyLock( mOutputQueueEmptyMutex );
					mIsOutputQueueEmpty = false;
				}
			}
			// send back the sync output
			outputPromise.set_value( outputStr );
		} );
	}

	// Create the async error read thread
	if( redirectError ) {
		mErrorReadThread = std::make_unique<std::thread>( [this]() {
			std::promise<std::string> errorPromise;
			mErrorFuture = errorPromise.get_future();
			std::string outputStr;
			DWORD readBytes;
			std::array<char, 65536> buffer;
			while( ReadFile( mErrorRead, buffer.data(), static_cast<DWORD>(65536), &readBytes, nullptr ) && readBytes != 0 ) {
				// copy the buffer into a string
				std::string error( buffer.begin(), buffer.begin() + readBytes );
				// add the string to the sync output
				outputStr += error;
				// push the string to the async queue
				std::lock_guard<std::mutex> queueLock( mErrorQueueMutex );
				bool wasEmpty = mErrorQueue.empty();
				mErrorQueue.push( error );
				if( wasEmpty ) {
					std::lock_guard<std::mutex> queueEmptyLock( mErrorQueueEmptyMutex );
					mIsErrorQueueEmpty = false;
				}
			}
			// send back the sync output
			errorPromise.set_value( outputStr );
		} );
	}

#else
#endif
}

Process::~Process()
{
	terminate();
}

std::string Process::getOutputSync()
{
	mOutputFuture.wait();
	return mOutputFuture.get();
}
std::string Process::getErrorSync()
{
	mErrorFuture.wait();
	return mErrorFuture.get();
}

std::string	Process::getOutputAsync()
{
	std::lock_guard<std::mutex> queueLock( mOutputQueueMutex );
	if( ! mOutputQueue.empty() ) {
		std::string output = mOutputQueue.front();
		mOutputQueue.pop();
		if( mOutputQueue.empty() ) {
			std::lock_guard<std::mutex> queueEmptyLock( mOutputQueueEmptyMutex );
			mIsOutputQueueEmpty = true;
		}
		return output;
	}
	return std::string();
}
std::string	Process::getErrorAsync()
{
	std::lock_guard<std::mutex> queueLock( mErrorQueueMutex );
	if( ! mErrorQueue.empty() ) {
		std::string error = mErrorQueue.front();
		mErrorQueue.pop();
		if( mErrorQueue.empty() ) {
			std::lock_guard<std::mutex> queueEmptyLock( mErrorQueueEmptyMutex );
			mIsErrorQueueEmpty = true;
		}
		return error;
	}
	return std::string();
}

bool Process::isOutputAvailable()
{
	std::lock_guard<std::mutex> queueEmptyLock( mOutputQueueEmptyMutex );
	return ! mIsOutputQueueEmpty;
}
bool Process::isErrorAvailable()
{
	std::lock_guard<std::mutex> queueEmptyLock( mErrorQueueEmptyMutex );
	return ! mIsErrorQueueEmpty;
}

bool Process::write( const std::string &cmd )
{
  if( mInputWrite ) {
    DWORD nBytesWritten;
    DWORD nBytesToWrite = static_cast<DWORD>( cmd.length() );
	if( WriteFile( mInputWrite, &cmd[0], nBytesToWrite, &nBytesWritten, NULL ) && nBytesWritten != 0 ) {
		return true;
    }
  }
  return false;
}

void Process::closeInput()
{
	if( mInputWrite ) {
		CloseHandle( mInputWrite );
		mInputWrite = nullptr;
	}
}

int16_t Process::terminate()
{
	// Start by closing the input if it's not 
	// closed already
	closeInput();

	// Then wait for the Process to exit
	int16_t exitCode = -1;
	if( mProcessRunning ) {
		WaitForSingleObject( mProcess, INFINITE );
		// And get its exit code
		DWORD exitCodeDWORD;
		if( GetExitCodeProcess( mProcess, &exitCodeDWORD ) ) {
			exitCode = static_cast<int16_t>( exitCodeDWORD );
		}
		// Close the Process Handle
		CloseHandle( mProcess );
		mProcessRunning = false;
	}

	// Join Threads
	if( mOutputReadThread && mOutputReadThread->joinable() ) {
		mOutputReadThread->join();
	}
	if( mErrorReadThread && mErrorReadThread->joinable() ) {
		mErrorReadThread->join();
	}

	// Close Handles
	if( mOutputRead ) {
		CloseHandle( mOutputRead );
		mOutputRead = nullptr;
	}
	if( mErrorRead ) {
		CloseHandle( mErrorRead );
		mErrorRead = nullptr;
	}

	return exitCode;
}

CompilerRef Compiler::create()
{
	return std::make_shared<Compiler>();
}
Compiler::Compiler()
: mVerbose( false )
{	
	// get the application name and directory
	mAppPath		= app::getAppPath().parent_path();
	mProjectPath	= mAppPath.parent_path().parent_path();
	mProjectName	= mProjectPath.stem().string();
	
	// find the compiler/linker arguments and start the process
	// with visual studio env vars and paths
	findAppBuildArguments();
	initializeProcess();

	// Connect the process redirection to cinder's console
	mProcessOutputConnection = app::App::get()->getSignalUpdate().connect( bind( &Compiler::parseProcessOutput, this ) );
}

Compiler::~Compiler()
{
	mProcess->terminate();
	mProcessOutputConnection.disconnect();
	for( const auto &path : mTemporaryFiles ) {
		if( fs::exists( path ) ) { 
			try {
				fs::remove( path );
			} catch( ... ) {}
		}
	}
}

namespace {
	inline std::string quote( const std::string &input ) { return "\"" + input + "\""; };
	inline std::string removeEndline( const std::string &input ) { return input.back() == '\n' ? input.substr( 0, input.length() - 1 ) : input; }
	
	// http://stackoverflow.com/questions/5343190/how-do-i-replace-all-instances-of-a-string-with-another-string
	void replaceAll( string& str, const string& from, const string& to ) 
	{
		if(from.empty())
			return;
		string wsRet;
		wsRet.reserve(str.length());
		size_t start_pos = 0, pos;
		while((pos = str.find(from, start_pos)) != string::npos) {
			wsRet += str.substr(start_pos, pos - start_pos);
			wsRet += to;
			pos += from.length();
			start_pos = pos;
		}
		wsRet += str.substr(start_pos);
		str.swap(wsRet); // faster than str = wsRet;
	}
	
	// http://stackoverflow.com/questions/1798112/removing-leading-and-trailing-spaces-from-a-string
	std::string trim( const std::string &str, const std::string &whitespace = " \t" )
	{
		const auto strBegin = str.find_first_not_of( whitespace );
		if( strBegin == std::string::npos )
			return ""; // no content

		const auto strEnd = str.find_last_not_of( whitespace );
		const auto strRange = strEnd - strBegin + 1;

		return str.substr( strBegin, strRange );
	}

	fs::path getNextAvailableName( const ci::fs::path &path ) 
	{
		auto parent = path.parent_path();
		auto stem = path.stem().string();
		auto ext = path.extension().string();
		uint16_t count = 0;
		while( fs::exists( parent / ( stem + "_" + to_string( count ) + ext ) ) ){
			count++;
		}
		return parent / ( stem + "_" + to_string( count ) + ext );
	}
} // anonymous namespace


//#define USE_PCH
//#define USE_SEMI_RANDOM_NAMES
//#define USE_DLL_ENTRY
//#define USE_FAST_LINK

void Compiler::parseProcessOutput()
{
	string fullOutput;
	while( mProcess->isOutputAvailable() ) {
		auto output = removeEndline( mProcess->getOutputAsync() );
		if( output.find( "error" ) != string::npos ) { 
			mErrors.push_back( output );	
		}
		else if( output.find( "warning" ) != string::npos ) {
			mWarnings.push_back( output );
		}
		if( output.find( "CI_BUILD_FINISHED" ) != string::npos ) {
			if( mBuildFinish ) {
				mBuildFinish( CompilationResult( mFilePath, mCurrentOutputPath, mErrors, mWarnings, mSymbols ) );
			}
		}
		fullOutput += output;
	}
	if( mVerbose && ! fullOutput.empty() ) { 
		app::console() << fullOutput << endl;
	}
}

ci::fs::path Compiler::resolvePath( const ci::fs::path &path )
{
	fs::path actualPath;
	if( fs::exists( path ) ) {
		actualPath = path;
	}
	else if( ! path.has_parent_path() && fs::exists( mProjectPath / "src" / path ) ) {
		actualPath = mProjectPath / "src" / path;
	}
	else if( ! path.has_parent_path() && fs::exists( mAppPath / path ) ) {
		actualPath = mAppPath / path;
	}
	else {
		throw CompilerException( "Can't find exact file path for " + path.string() );
	}
	return actualPath;
}

void Compiler::build( const ci::fs::path &path, const std::function<void(const CompilationResult&)> &onBuildFinish )
{
	build( path, Options(), onBuildFinish );
}

void Compiler::build( const ci::fs::path &path, const Compiler::Options &options, const std::function<void(const CompilationResult&)> &onBuildFinish )
{
	if( ! mProcess ) {
		throw CompilerException( "Compiler Process not initialized" );
	}

	// set options
	mVerbose = options.mVerbose;
	auto compileArgs = options.mCompileArgs.empty() ? mCompileArgs : options.mCompileArgs;
	auto linkArgs = options.mLinkArgs.empty() ? mLinkArgs : options.mLinkArgs;
	auto precompiledHeader = ! options.mPrecompiledHeader.empty() ? options.mPrecompiledHeader : "";

	// clear the error and warning vectors
	mErrors.clear();
	mWarnings.clear();
	
	// try to find the actual path
	fs::path actualPath = resolvePath( path );
	mFilePath = actualPath;

	// create the build command
	std::string command;
	auto outputPath = options.mOutputPath.empty() ? mAppPath : options.mOutputPath;
	auto libName = quote( ( outputPath / ( path.stem().string() + ".lib" ) ).string() );
	auto defName = quote( ( outputPath / ( path.stem().string() + ".def" ) ).string() );
	auto dllName = quote( ( outputPath / ( path.stem().string() + ".dll" ) ).string() );
	auto objName = quote( ( outputPath / ( path.stem().string() + ".obj" ) ).string() );
	auto pdbName = quote( ( outputPath / ( path.stem().string() + ".pdb" ) ).string() );
	auto pchName = ! precompiledHeader.empty() ? quote( ( outputPath / ( precompiledHeader.stem().string() + ".pch" ) ).string() ) : "";
	mCurrentOutputPath = outputPath / ( path.stem().string() + ".dll" );
	
	// try deleting or renaming previous pdb files to prevent errors
	if( fs::exists( outputPath / ( path.stem().string() + ".pdb" ) ) ) {
		bool deleted = false;
		try {
			fs::remove( outputPath / ( path.stem().string() + ".pdb" ) );
			deleted = true;
		}
		catch( ... ) {}
		if( ! deleted ) {
			auto newName = getNextAvailableName( outputPath / ( path.stem().string() + ".pdb" ) );
			try {
				fs::rename( outputPath / ( path.stem().string() + ".pdb" ), newName );
				//pdbName = newName.string();
				mTemporaryFiles.push_back( newName );
			} catch( ... ) {}
		}
	}

	// compile the precompiled-header
	//_____________________________________________
	if( options.mCreatePrecompiledHeader && ! precompiledHeader.empty() ) {
		command += "cl /c " + mCompileArgs;
		// add defines
		for( const auto &def : options.mPpDefinitions ) {
			command += " /D " + def;
		}
		// includes
		for( const auto &include : options.mIncludes ) {
			command += " /I " + include;
		}
		//command += " /Fd" + pdbName;
		//command += " /FS";
		//command += " /Gz";
		command += " /cgthreads4";
		command += " /Fo" + outputPath.string() + "\\"; 
		//command += " /Fo" + ( outputPath / ( precompiledHeader.stem().string() + ".obj" ) ).string();
		command += " /Fp" + pchName;
		command += " /Yc" + precompiledHeader.stem().string() + ".h";
		command += " " + quote( resolvePath( precompiledHeader ).string() );
		command += "\n";
	}

	// additional files to compile
	//_____________________________________________
	if( options.mCompileList.size() ) {
		command += "cl " + mCompileArgs;
		// add defines
		for( const auto &def : options.mPpDefinitions ) {
			command += " /D " + def;
		}
		// includes
		for( const auto &include : options.mIncludes ) {
			command += " /I " + include;
		}
		command += " /cgthreads4";
		//command += " /Fd" + pdbName;
		if( ! precompiledHeader.empty() ) {
			command += " /Yu" + precompiledHeader.stem().string() + ".h";; // Precompiled Header
			command += " /Fp" + pchName;
		}
		for( const auto &path : options.mCompileList ) {
			command += " " + quote( resolvePath( path ).string() );
		}
		command += "/n";
	}

	// compile the actual file
	//_____________________________________________
	command += "cl " + mCompileArgs;
	// add defines
	for( const auto &def : options.mPpDefinitions ) {
		command += " /D " + def;
	}
	// includes
	for( const auto &include : options.mIncludes ) {
		command += " /I " + include;
	}
	command += " /cgthreads4";
	//command += " /Fd" + pdbName;
	//command += " /FS";
	//command += " /Bt";
	//command += " /Gz";
	command += " /Fo" + outputPath.string() + "\\"; 
	if( ! precompiledHeader.empty() ) {// && ! options.mCreatePrecompiledHeader ) {
		command += " /Yu" + precompiledHeader.stem().string() + ".h"; // Use Precompiled Header
		command += " /Fp" + pchName;
	}
	for( const auto &inc : options.mForcedIncludes ) {
		command += " /FI " + inc;
	}
	//else if( options.mCreatePrecompiledHeader ) {
	//	command += " /Yc" + precompiledHeader.stem().string() + ".h"; // Create Precompiled Header
	//	command += " /Fp" + pchName;
	//	command += " " + quote( resolvePath( precompiledHeader ).string() );
	//	app::console() << "ICI: " << quote( resolvePath( precompiledHeader ).string() ) << endl;
	//}
	//command += " /YX /Yl-";
#if defined( USE_DLL_ENTRY )
	command += " " + quote( ( mProjectPath / "src" / "dllmain.cpp" ).string() );
#endif
	command += " " + quote( actualPath.string() );
	//command += "\n";
	
	// linker
	//_____________________________________________
	command += " /link " + mLinkArgs;
	command += " /OUT:" + dllName;
	command += " /IMPLIB:" + libName;
	command += " /PDB:" + pdbName;
	
#if ! defined( USE_PCH ) && defined( USE_FAST_LINK )
	//command += " /DEBUG:FASTLINK";
#endif
	command += " /DLL ";// + objName;
#if defined( USE_DLL_ENTRY )
	command += " " + quote( ( outputPath / "dllmain.obj" ).string() );
#endif
	
	if( ! precompiledHeader.empty() ) {
		command += " " + quote( ( outputPath / ( precompiledHeader.stem().string() + ".obj" ) ).string() );
	}
	// additional files to link
	for( const auto &path : options.mLinkList ) {
		command += " " + quote( resolvePath( path ).string() );
	}
	for( const auto &path : options.mCompileList ) {
		if( fs::exists( outputPath / ( path.stem().string() + ".obj" ) ) ) {
			command += " " + quote( ( outputPath / ( path.stem().string() + ".obj" ) ).string() );
		}
	}
	command += "\n";

	// dump and parse symbols
	//_____________________________________________
	if( options.mDumpSymbols ) {
		command += "dumpbin /EXPORTS " + dllName + " /OUT:" + defName;
		mSymbols.clear();
		std::wifstream defFile( ( outputPath / ( path.stem().string() + ".def" ) ).string() );
		bool reachedSymbols = false;
		for( wstring line; getline( defFile, line ); ) {
			if( line.find( L"    ordinal hint RVA" ) != wstring::npos ) {
				reachedSymbols = true;
				continue;
			}
			if( line.find( L"  Summary" ) != wstring::npos ) {
				break;
			}
			if( reachedSymbols && line.length() > 26 ) {
				// extract symbol and mangled name
				auto symbolLine = line.substr( 26 );
				auto symbol = symbolLine.substr( 0, symbolLine.find_first_of( '=' ) - 1 );
				auto signature = symbolLine.substr( symbolLine.find_first_of( '(' ) + 1 );
				signature = signature.substr( 0, signature.find_last_of( ')' ) );
				// try to cleanup symbol string
				auto signatureStr = string( signature.begin(), signature.end() );
				replaceAll( signatureStr, "public: ", "" );
				replaceAll( signatureStr, "__thiscall ", "" );
				replaceAll( signatureStr, "__cdecl ", "" );
				replaceAll( signatureStr, "(void)", "()" );
				replaceAll( signatureStr, "class ", "" );
				replaceAll( signatureStr, "std::basic_string<char,struct std::char_traits<char>,std::allocator<char> >", "std::string" );
				replaceAll( signatureStr, ")const", ") const" );
				signatureStr = trim( signatureStr );
				auto symbolStr = trim( string( symbol.begin(), symbol.end() ) );
				mSymbols.insert( make_pair( signatureStr, symbolStr ) );
			}
		}
	}
	
	// issue the build command with a completion token
	//_____________________________________________
	mBuildFinish = onBuildFinish;
	mProcess << command << endl << "CI_BUILD_FINISHED:" << actualPath.filename() << endl;
}

void Compiler::initializeProcess()
{
	if( fs::exists( "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat" ) ) {
		// create a cmd process with the right environment variables and paths
		mProcess = make_unique<Process>( "cmd /k prompt 1$g\n", mAppPath.parent_path().string(), true, true );
		mProcess << quote( "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat" ) + " x86" << endl;
	}
	else {
		throw CompilerException( "Failed Initializing Compiler Process" );
	}
}

void Compiler::findAppBuildArguments()
{
	// Helper to remove unecessary arguments
	const auto cleanArguments = []( const std::string &command, const std::vector<std::string> &argsToIgnore ) {
		std::string newCommand;
		std::vector<std::string> args = split( command, "/" );
		for( size_t i = 0; i < args.size(); ++i ) {
			bool ignoreArg = false;
			for( const auto &ignore : argsToIgnore ) {
				if( ! args[i].compare( 0, ignore.length(), ignore ) ) {
					ignoreArg = true;		
				}
			}
			// only keep arguments that should not be ignored
			if( ! ignoreArg && ! args[i].empty() ) {
				// because of the split( command, "/" ), the last line
				// is likely to contain an unwanted list of files to compile/link
				if( i == args.size() - 1 ) {
					auto firstSpace = args[i].find_first_of( " " );
					newCommand+= "/" + args[i].substr( 0, firstSpace );
				}
				else {
					newCommand += "/" + args[i];
				}
			}
		}
		return newCommand;
	};

	//// try to get the compiler and linker commands from vs tlog folder
	auto logPath = mAppPath / ( mProjectName + ".tlog" );
	if( fs::exists( logPath ) ) {
		// compiler commands
		if( mCompileArgs.empty() ) {
			auto clCommandLog = logPath / "CL.command.1.tlog";
			if( fs::exists( clCommandLog ) ) {
				// .tlog file need to be read as unicode
				std::wifstream infile( clCommandLog.string() );
				infile.imbue( std::locale( infile.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>() ) );
				// The first line not starting with ^ should be a good compiler arg list to use
				for( std::wstring line; std::getline( infile, line ); ) {
					if( line[0] != L'^' ){
						mCompileArgs = string( line.begin(), line.end() );
						break;
					}
				}
				// if a cl.command log was found get rid of the unecessary compiler arguments
				if( ! mCompileArgs.empty() ){
					mCompileArgs = cleanArguments( mCompileArgs, 
					{ "c", "Fd", "Fo" //, "Zi", "Gd" // Doesn't play well with precompiled header
					} );
				}
			}
		}
		// linker commands
		if( mLinkArgs.empty() ) {
			auto linkCommandLog = logPath / "link.command.1.tlog";
			if( fs::exists( linkCommandLog ) ) {
				// .tlog file need to be read as unicode
				std::wifstream infile( linkCommandLog.string() );
				infile.imbue( std::locale( infile.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>() ) );
				// The first line not starting with ^ should be a good compiler arg list to use
				for( std::wstring line; std::getline( infile, line ); ) {
					if( line[0] != L'^' ){
						mLinkArgs = string( line.begin(), line.end() );
						break;
					}
				}
				// if a link.command log was found get rid of the unecessary compiler arguments
				if( ! mLinkArgs.empty() ) {
					mLinkArgs = cleanArguments( mLinkArgs, { "OUT", "PDB", "IMPLIB", "SUBSYSTEM" } );
				}
			}
		}
	}
}

ci::fs::path CompilationResult::getFilePath() const
{
	return mFilePath;
}
ci::fs::path CompilationResult::getOutputPath() const
{
	return mOutputPath;
}
bool CompilationResult::hasErrors() const
{
	return ! mErrors.empty();
}
const std::vector<std::string>& CompilationResult::getErrors() const
{
	return mErrors;
}
const std::vector<std::string>& CompilationResult::getWarnings() const
{
	return mWarnings;
}

const std::map<std::string,std::string>& CompilationResult::getSymbols() const
{
	return mSymbols;
}

CompilationResult::CompilationResult( const ci::fs::path &filePath, const ci::fs::path &outputPath, const std::vector<std::string> &errors, const std::vector<std::string> &warnings, const std::map<std::string,std::string> &symbols )
: mFilePath( filePath ), mOutputPath( outputPath ), mErrors( errors ), mWarnings( warnings ), mSymbols( symbols )	
{
}

} // namespace runtime