#include "runtime/Process.h"

#if ! defined( WIN32_LEAN_AND_MEAN )
	#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <array>

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
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
  if( mInputWrite ) {
    DWORD nBytesWritten;
    DWORD nBytesToWrite = static_cast<DWORD>( cmd.length() );
	if( WriteFile( mInputWrite, &cmd[0], nBytesToWrite, &nBytesWritten, NULL ) && nBytesWritten != 0 ) {
		return true;
    }
  }
#else
#endif
  return false;
}

void Process::closeInput()
{
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	if( mInputWrite ) {
		CloseHandle( mInputWrite );
		mInputWrite = nullptr;
	}
#else
#endif
}

int16_t Process::terminate()
{
	// Start by closing the input if it's not 
	// closed already
	closeInput();

	// Then wait for the Process to exit
	int16_t exitCode = -1;
	
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
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
#endif

	// Join Threads
	if( mOutputReadThread && mOutputReadThread->joinable() ) {
		mOutputReadThread->join();
	}
	if( mErrorReadThread && mErrorReadThread->joinable() ) {
		mErrorReadThread->join();
	}

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	// Close Handles
	if( mOutputRead ) {
		CloseHandle( mOutputRead );
		mOutputRead = nullptr;
	}
	if( mErrorRead ) {
		CloseHandle( mErrorRead );
		mErrorRead = nullptr;
	}
#endif

	return exitCode;
}