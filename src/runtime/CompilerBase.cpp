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
}

CompilerBase::CompilerBase()
{
}

CompilerBase::~CompilerBase()
{
}

void CompilerBase::initializeProcess()
{
	//if( fs::exists( getCompilerPath() ) ) {
		
		// create a command line process with the right environment variables and paths
		mProcess = make_unique<Process>( "cmd /k prompt 1$g\n", getProcessPath().string(), true, true );
		
		// start the compiler process
		mProcess << getCompilerPath().string() << endl;
		mProcess << "cl" << endl;
		// Connect the process redirection to cinder's console
		//mProcessOutputConnection = 
		app::App::get()->getSignalUpdate().connect( [this](){
			string fullOutput;
			while( mProcess->isOutputAvailable() ) {
				auto output = removeEndline( mProcess->getOutputAsync() );
				/*if( output.find( "error" ) != string::npos ) { 
					mErrors.push_back( output );	
				}
				else if( output.find( "warning" ) != string::npos ) {
					mWarnings.push_back( output );
				}
				if( output.find( "CI_BUILD_FINISHED" ) != string::npos ) {
					if( mBuildFinish ) {
						mBuildFinish( CompilationResult( mFilePath, mCurrentOutputPath, mErrors, mWarnings, mSymbols ) );
					}
				}*/
				if( output != "\r" ) {
					fullOutput += output;
				}
			}
			if( ! fullOutput.empty() ) app::console() << fullOutput << endl;
			});
	//}
	//else {
	//	throw CompilerException( "Failed Initializing Compiler Process" );
	//}
}


std::string CompilerBase::getCLInitCommand() const
{
	return mCLInitCommand;
}

ci::fs::path CompilerBase::getCompilerPath() const
{
	return mCompilerPath;
}

ci::fs::path CompilerBase::getProcessPath() const
{
	return mProcessPath;
}

}