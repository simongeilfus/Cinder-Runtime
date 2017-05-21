#include "runtime/CompilerMSVC.h"
#include "runtime/Process.h"
#include "cinder/app/App.h"

using namespace std;
using namespace ci;

namespace runtime {

namespace {
	inline std::string quote( const std::string &input ) { return "\"" + input + "\""; };
}

CompilerMSVCRef CompilerMSVC::create()
{
	return make_shared<CompilerMSVC>();
}

CompilerMSVC::CompilerMSVC()
{
	initializeProcess();
}

CompilerMSVC::~CompilerMSVC()
{
}

void CompilerMSVC::build( const std::string &arguments, const std::function<void( const CompilationResult& )> &onBuildFinish )
{
	if( ! mProcess ) {
		throw CompilerException( "Compiler Process not initialized" );
	}
	// clear the error and warning vectors
	mErrors.clear();
	mWarnings.clear();
		
	// issue the build command with a completion token
	mBuildFinish = onBuildFinish;
	mProcess << arguments << endl << "CI_BUILD_FINISHED" << endl;
}

std::string CompilerMSVC::getCLInitCommand() const
{
	return "cmd /k prompt 1$g\n";
}

ci::fs::path CompilerMSVC::getCompilerPath() const
{
	return "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat";
}

std::string CompilerMSVC::getCompilerInitArgs() const
{
#ifdef _WIN64
	return " x64";
#else
	return " x86";
#endif
}

ci::fs::path CompilerMSVC::getProcessPath() const
{
	auto appPath = app::getAppPath().parent_path();
	return appPath.parent_path().parent_path().parent_path().string();
}


}