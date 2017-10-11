#include "runtime/CompilerBase.h"
#include "runtime/Process.h"

#include "cinder/app/App.h"
#include "cinder/Filesystem.h"

using namespace std;
using namespace ci;

namespace runtime {

namespace {
	inline std::string removeEndline( const std::string &input ) 
	{ 
		auto output = input;
		while( output.size() && ( output.back() == '\n' ) ) output = output.substr( 0, output.length() - 1 );
		return output;
	}
	inline std::string removeCarriageReturn( const std::string &input ) 
	{ 
		auto output = input;
		while( output.size() && ( output.back() == '\r' ) ) output = output.substr( 0, output.length() - 1 );
		return output;
	}
	inline std::string quote( const std::string &input ) { return "\"" + input + "\""; };
}

CompilerBase::CompilerBase()
	: mVerbose( false )
{
}

CompilerBase::~CompilerBase()
{
}

void CompilerBase::parseProcessOutput()
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
		fullOutput += output;
	}
	if( mVerbose && ! fullOutput.empty() ) app::console() << fullOutput << endl;
}

void CompilerBase::initializeProcess()
{
	if( fs::exists( getCompilerPath() ) ) {
		
		// create a command line process with the right environment variables and paths
		mProcess = make_unique<Process>( getCLInitCommand(), getCLInitPath().string(), true, true );
		
		// start the compiler process
		mProcess << quote( getCompilerPath().string() ) + " " + getCompilerInitArgs() << endl;
		
		// Connect the process redirection to cinder's console
		mProcessOutputConnection = app::App::get()->getSignalUpdate().connect( bind( &CompilerBase::parseProcessOutput, this ) );
	}
	else {
		throw CompilerException( "Failed Initializing Compiler Process at " + getCompilerPath().string() );
	}
}

CompilationResult::CompilationResult()
{
}

ci::fs::path CompilationResult::getOutputPath() const
{
	return mOutputPath;
}
const std::vector<ci::fs::path>& CompilationResult::getFilePaths() const
{
	return mFilePaths;
}
std::vector<ci::fs::path>& CompilationResult::getFilePaths()
{
	return mFilePaths;
}
const std::vector<ci::fs::path>& CompilationResult::getObjectFilePaths() const
{
	return mObjectFilePaths;
}
std::vector<ci::fs::path>& CompilationResult::getObjectFilePaths()
{
	return mObjectFilePaths;
}
ci::fs::path CompilationResult::getPdbFilePath() const
{
	return mPdbFilePath;
}
bool CompilationResult::hasErrors() const
{
	return ! mErrors.empty();
}
const std::vector<std::string>& CompilationResult::getErrors() const
{
	return mErrors;
}
std::vector<std::string>& CompilationResult::getErrors()
{
	return mErrors;
}
const std::vector<std::string>& CompilationResult::getWarnings() const
{
	return mWarnings;
}
std::vector<std::string>& CompilationResult::getWarnings()
{
	return mWarnings;
}
void CompilationResult::setOutputPath( const ci::fs::path &path )
{
	mOutputPath = path;
}
void CompilationResult::setPdbFilePath( const ci::fs::path &path )
{
	mPdbFilePath = path;
}
}