#include "runtime/CompilerMSVC.h"
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
	mCLInitCommand	= "cmd /k prompt 1$g\n";
#ifdef _WIN64
	mCompilerPath	= quote( "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat" ) + " x64";
#else
	mCompilerPath	= quote( "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat" ) + " x86";
#endif
	auto mAppPath	= app::getAppPath().parent_path();
	mProcessPath	= mAppPath.parent_path().parent_path().parent_path().string();

	initializeProcess();
}

CompilerMSVC::~CompilerMSVC()
{
}

}