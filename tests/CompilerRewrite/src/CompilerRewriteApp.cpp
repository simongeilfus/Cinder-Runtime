#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "runtime/CompilerMSVC.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CompilerRewriteApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;

	rt::CompilerRef mCompiler;
};

void CompilerRewriteApp::setup()
{
	mCompiler = rt::Compiler::create();
	
	// compiler args
	std::string command = "cl ";
	command += " /Fd\"C:\\CODE\\CINDER\\CINDER_FORK\\BLOCKS\\CINDER-RUNTIME\\TESTS\\COMPILERREWRITE\\VC2015\\BUILD\\X64\\DEBUG_SHARED\\INTERMEDIATE\\VC140_RT.PDB\" ";
	command += " /Fo\"C:\\CODE\\CINDER\\CINDER_FORK\\BLOCKS\\CINDER-RUNTIME\\TESTS\\COMPILERREWRITE\\VC2015\\BUILD\\X64\\DEBUG_SHARED\\INTERMEDIATE\\\\\" "; 
	command += "/Zi /nologo /W3 /WX- /Od /D CINDER_SHARED /D WIN32 /D _WIN32_WINNT=0x0601 /D _WINDOWS /D NOMINMAX /D _DEBUG /D _UNICODE /D UNICODE /Gm /EHsc /RTC1 /MDd /GS /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /Gd /TP ";
	command += "/I..\\INCLUDE /I..\\..\\..\\..\\..\\INCLUDE /I..\\..\\..\\INCLUDE /I..\\BLOCKS\\WATCHDOG\\INCLUDE ";
	command += " C:\\CODE\\CINDER\\CINDER_FORK\\BLOCKS\\CINDER-RUNTIME\\TESTS\\COMPILERREWRITE\\SRC\\TEST.CPP";

	// linker args
	command += " /link ";
	command += " /OUT:BUILD\\X64\\DEBUG_SHARED\\INTERMEDIATE\\test.dll";
	command += " /IMPLIB:BUILD\\X64\\DEBUG_SHARED\\INTERMEDIATE\\test.lib";
	command += " /INCREMENTAL:NO /NOLOGO";
	command += " /LIBPATH:..\\..\\..\\..\\..\\LIB\\MSW\\X64";
	command += " /LIBPATH:..\\..\\..\\..\\..\\LIB\\MSW\\X64\\DEBUG_SHARED\\V140\\";
	command += " CINDER.LIB OPENGL32.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB /NODEFAULTLIB:LIBCMT /NODEFAULTLIB:LIBCPMT ";
	command += " /MACHINE:X64";
#if defined( _DEBUG )
	command += " /PDB:\"C:\\CODE\\CINDER\\CINDER_FORK\\BLOCKS\\CINDER-RUNTIME\\TESTS\\COMPILERREWRITE\\VC2015\\BUILD\\X64\\DEBUG_SHARED\\INTERMEDIATE\\VC140_RT.PDB\"";
	command += " /DEBUG:FASTLINK";
#endif
	command += " /DLL ";

	mCompiler->build( command, []( const rt::CompilationResult &result ) {
		app::console() << "Done!" << endl;
	} );
}

void CompilerRewriteApp::update()
{
}

void CompilerRewriteApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( CompilerRewriteApp, RendererGl() )
