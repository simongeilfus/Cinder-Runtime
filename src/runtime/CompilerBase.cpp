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
	: mVerbose( true )
{
}

CompilerBase::~CompilerBase()
{
}

void CompilerBase::parseProcessOutput()
{
	string fullOutput;
	bool done = false;
	while( mProcess->isOutputAvailable() ) {
		auto output = removeEndline( mProcess->getOutputAsync() );
		if( output.find( "error" ) != string::npos ) { 
			mErrors.push_back( output );	
		}
		else if( output.find( "warning" ) != string::npos ) {
			mWarnings.push_back( output );
		}
		if( output.find( "CI_BUILD_FINISHED" ) != string::npos ) {
			done = true;
		}
		fullOutput += output;
	}
	if( mVerbose && ! fullOutput.empty() ) app::console() << fullOutput << endl;
	
	if( done && mBuildFinish ) {
		mBuildFinish( CompilationResult( "", "", mErrors, mWarnings, { { "", "" } } ) );
	}
}

void CompilerBase::initializeProcess()
{
	if( fs::exists( getCompilerPath() ) ) {
		
		// create a command line process with the right environment variables and paths
		mProcess = make_unique<Process>( getCLInitCommand(), getProcessPath().string(), true, true );
		
		// start the compiler process
		mProcess << quote( getCompilerPath().string() ) + " " + getCompilerInitArgs() << endl;
		
		// Connect the process redirection to cinder's console
		mProcessOutputConnection = app::App::get()->getSignalUpdate().connect( bind( &CompilerBase::parseProcessOutput, this ) );
	}
	else {
		throw CompilerException( "Failed Initializing Compiler Process at " + getCompilerPath().string() );
	}
}

}