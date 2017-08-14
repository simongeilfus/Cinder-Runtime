#include "runtime/CompilerMsvc.h"
#include "runtime/ClassFactory.h"
#include "runtime/PrecompiledHeader.h"
#include "runtime/Process.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"

using namespace std;
using namespace ci;

namespace runtime {

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::default( )
{
	return include( CI_RT_PROJECT_ROOT / "include" )
		.include( CI_RT_PROJECT_ROOT / "src" )
		.include( CI_RT_CINDER_PATH / "include" )
		.libraryPath( CI_RT_CINDER_PATH / "lib/msw" / CI_RT_PLATFORM )
		.libraryPath( CI_RT_CINDER_PATH / "lib/msw" / CI_RT_PLATFORM / CI_RT_CONFIGURATION / CI_RT_PLATFORM_TOOLSET )
		.library( "cinder.lib" )
		.define( "CINDER_SHARED" ).define( "WIN32" ).define( "_WIN32_WINNT=0x0601" ).define( "_WINDOWS" ).define( "NOMINMAX" ).define( "_UNICODE" ).define( "UNICODE" )
		.compilerOption( "/nologo" ).compilerOption( "/W3" ).compilerOption( "/WX-" ).compilerOption( "/EHsc" ).compilerOption( "/RTC1" ).compilerOption( "/GS" ).compilerOption( "/fp:precise" ).compilerOption( "/Zc:wchar_t" ).compilerOption( "/Zc:forScope" ).compilerOption( "/Zc:inline" ).compilerOption( "/Gd" ).compilerOption( "/TP" )
		.compilerOption( "/Gm" )
#if defined( _DEBUG )
		.compilerOption( "/Od" )
		.compilerOption( "/Zi" )
		.define( "_DEBUG" )
		.compilerOption( "/MDd" )
#else
		.compilerOption( "/MD" )
#endif
		.linkerOption( "/INCREMENTAL:NO" )
		.linkerOption( "/NOLOGO" ).linkerOption( "/NODEFAULTLIB:LIBCMT" ).linkerOption( "/NODEFAULTLIB:LIBCPMT" )
		;
}

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::include( const ci::fs::path &path )
{
	mIncludes.push_back( path );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::libraryPath( const ci::fs::path &path )
{
	mLibraryPaths.push_back( path );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::library( const std::string &library )
{
	mLibraries.push_back( library );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::define( const std::string &definition )
{
	mPpDefinitions.push_back( definition );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::precompiledHeader( const ci::fs::path &path, bool create )
{
	mCreatePrecompiledHeader = true;
	mPrecompiledHeader = path;
	return *this;
}

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::objectFile( const ci::fs::path &path )
{
	mObjectFilePath = path;
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::programDatabase( const ci::fs::path &path )
{
	mPdbPath = path;
	return *this;
}

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::compilerOption( const std::string &option )
{
	mCompilerOptions.push_back( option );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::linkerOption( const std::string &option )
{
	mLinkerOptions.push_back( option );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::verbose( bool enabled )
{
	mVerbose = enabled;
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::outputPath( const ci::fs::path &path )
{
	mOutputPath = path;
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::forceInclude( const std::string &filename )
{
	mForcedIncludes.push_back( filename );
	return *this;
}

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::additionalSource( const ci::fs::path &cppFile )
{
	mAdditionalSources.push_back( cppFile );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::additionalSources( const std::vector<ci::fs::path> &cppFiles )
{
	mAdditionalSources.insert( mAdditionalSources.begin(), cppFiles.begin(), cppFiles.end() );
	return *this;
}

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::linkObj( const ci::fs::path &path )
{
	mObjPaths.push_back( path );
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::linkAppObjs( bool link )
{
	mLinkAppObjs = link;
	return *this;
}

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::generateFactory( bool generate )
{
	mGenerateFactory = generate;
	return *this;
}

CompilerMsvc::BuildSettings::BuildSettings()
: mLinkAppObjs( true ), mGenerateFactory( true )
{
}

CompilerMsvc::CompilerMsvc()
{
	initializeProcess();
}

CompilerMsvc::~CompilerMsvc()
{
}

CompilerMsvc& CompilerMsvc::instance()
{
	static CompilerMsvcPtr compiler = make_unique<CompilerMsvc>();
	return *compiler.get();
}

void CompilerMsvc::build( const std::string &arguments, const std::function<void( const CompilationResult& )> &onBuildFinish )
{
	if( ! mProcess ) {
		throw CompilerException( "Compiler Process not initialized" );
	}
	// clear the error and warning vectors
	mErrors.clear();
	mWarnings.clear();
		
	// issue the build command with a completion token
	mProcess << arguments << endl << "CI_BUILD_FINISHED" << endl;
}

std::string CompilerMsvc::getCLInitCommand() const
{
	return "cmd /k prompt 1$g\n";
}

ci::fs::path CompilerMsvc::getCompilerPath() const
{
	return "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat";
}

std::string CompilerMsvc::getCompilerInitArgs() const
{
#ifdef _WIN64
	return " x64";
#else
	return " x86";
#endif
}

std::string CompilerMsvc::generateCompilerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings ) const
{
	string command = "cl ";
	for( auto define : settings.mPpDefinitions ) {
		command += "/D " + define + " ";
	}
	for( auto include : settings.mIncludes ) {
		command += "/I" + include.generic_string() + " ";
	}
	for( auto include : settings.mForcedIncludes ) {
		command += "/FI " + include + " ";
	}
	for( auto compilerArg : settings.mCompilerOptions ) {
		command += compilerArg + " ";
	}

	command += settings.mObjectFilePath.empty() ? "/Fo" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "/" ).string() + " " : "/Fo" + settings.mObjectFilePath.generic_string() + " ";
#if defined( _DEBUG )
	command += settings.mPdbPath.empty() ? "/Fd" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "/" ).string() + " " : "/Fd" + settings.mPdbPath.generic_string() + " ";
#endif
	
	// main source file
	command += sourcePath.generic_string() + " ";
	// additional files to compile
	for( const auto &path : settings.mAdditionalSources ) {
		command += path.generic_string() + " ";
	}

	return command;
}
std::string CompilerMsvc::generateLinkerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings ) const
{
	string command = "/link ";
	
	for( auto libraryPath : settings.mLibraryPaths ) {
		command += "/LIBPATH:" + libraryPath.generic_string() + " ";
	}
	for( auto library : settings.mLibraries ) {
		command += library + " ";
	}
	for( auto linkerArg : settings.mLinkerOptions ) {
		command += linkerArg + " ";
	}
	
	command += settings.mOutputPath.empty() ? "/OUT:" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + ".dll" ) ).string() + " " : "/OUT:" + settings.mOutputPath.generic_string() + " ";
#if defined( _DEBUG )
	command += "/DEBUG:FASTLINK ";
	command += settings.mPdbPath.empty() ? "/PDB:" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + ".pdb" ) ).string() + " " : "/PDB:" + settings.mPdbPath.generic_string() + " ";
	command += settings.mPdbPath.empty() ? "/PDBALTPATH:" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + ".pdb" ) ).string() + " " : "/PDBALTPATH:" + settings.mPdbPath.generic_string() + " ";
#endif
	command += "/DLL ";

	for( auto obj : settings.mObjPaths ) {
		command += obj.generic_string() + " ";
	}
	if( settings.mLinkAppObjs ) {
		for( auto it = fs::directory_iterator( CI_RT_INTERMEDIATE_DIR ), end = fs::directory_iterator(); it != end; it++ ) {
			if( it->path().extension() == ".obj" ) {
				// Skip obj for current source and current app
				if( it->path().filename().string().find( sourcePath.stem().string() + ".obj" ) == string::npos 
					&& it->path().filename().string().find( CI_RT_PROJECT_PATH.stem().string() + "App.obj" ) == string::npos
					) {
					command += it->path().generic_string() + " ";
				}
			}
		}
	}

	return command;
}

std::string CompilerMsvc::generateBuildCommand( const ci::fs::path &sourcePath, const BuildSettings &settings ) const
{
	return generateCompilerCommand( sourcePath, settings ) + generateLinkerCommand( sourcePath, settings );
}

namespace {
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


void CompilerMsvc::build( const ci::fs::path &sourcePath, const BuildSettings &settings, const std::function<void(const CompilationResult&)> &onBuildFinish )
{
	if( ! mProcess ) {
		throw CompilerException( "Compiler Process not initialized" );
	}
	// clear the error and warning vectors
	mErrors.clear();
	mWarnings.clear();

	// make sure the intermediate directories exists
	if( ! fs::exists( CI_RT_INTERMEDIATE_DIR / "runtime" ) ) {
		fs::create_directory( CI_RT_INTERMEDIATE_DIR / "runtime" );
	}
	if( ! fs::exists( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() ) ) {
		fs::create_directory( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() );
	}

#if defined( _DEBUG ) && 1
	// try renaming previous pdb files to prevent errors
	auto pdb = settings.mPdbPath.empty() ? ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + ".pdb" ) ) : settings.mPdbPath;
	if( fs::exists( pdb ) ) {
		auto newName = getNextAvailableName( pdb );
		try {
			fs::rename( pdb, newName );
		} catch( const std::exception & ) {}
	}
#endif

	// generate factor if needed and add it to the compiler list
	auto buildSettings = settings;
	if( settings.mGenerateFactory ) {
		auto factoryPath = CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + "Factory.cpp" );
		if( ! fs::exists( factoryPath ) ) {
			generateClassFactory( factoryPath, sourcePath.stem().string() );
		}
		auto factoryObjPath = CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + "Factory.obj" );
		if( ! fs::exists( factoryObjPath ) ) {
			buildSettings.additionalSource( factoryPath );
		}
		else {
			buildSettings.linkObj( factoryObjPath );
		}
	}
		
	// issue the build command with a completion token
	auto command = generateBuildCommand( sourcePath, buildSettings );
	mBuilds.insert( make_pair( sourcePath.filename(), onBuildFinish ) );
	app::console() << "------ rt::Compiler Build started: " << sourcePath.filename() << " ------" << endl;
	mProcess << command << endl << ( "CI_BUILD " + sourcePath.filename().string() ) << endl;

}

void CompilerMsvc::build( const std::vector<ci::fs::path> &sourcesPaths, const BuildSettings &settings, const std::function<void( const CompilationResult& )> &onBuildFinish )
{
	if( sourcesPaths.size() > 1 ) {
		BuildSettings buildSettings = settings;
		for( size_t i = 1; i < sourcesPaths.size(); ++i ) {
			buildSettings.additionalSource( sourcesPaths[i] );
		}
		build( sourcesPaths.front(), buildSettings, onBuildFinish );
	}
	else if( sourcesPaths.size() > 0 ) {
		build( sourcesPaths.front(), settings, onBuildFinish );
	}
}


namespace {
	inline std::string removeEndline( const std::string &input ) 
	{ 
		auto output = input;
		while( output.size() && ( output.back() == '\n' ) ) output = output.substr( 0, output.length() - 1 );
		return output;
	}
}

void CompilerMsvc::parseProcessOutput()
{
	string fullOutput;
	auto buildIt = mBuilds.end();
	while( mProcess->isOutputAvailable() ) {
		auto output = removeEndline( mProcess->getOutputAsync() );
		if( output.find( "error" ) != string::npos ) { 
			mErrors.push_back( output );	
		}
		else if( output.find( "warning" ) != string::npos ) {
			mWarnings.push_back( output );
		}
		if( output.find( "CI_BUILD" ) != string::npos ) {
			//app::console() << "Found CI_BUILD: " << output.substr( output.find_first_of( " " ) + 1 ) << endl;
			buildIt = mBuilds.find( output.substr( output.find_first_of( " " ) + 1 ) );
		}
		fullOutput += output;
	}
	
	if( mVerbose && ! fullOutput.empty() ) app::console() << fullOutput << endl;
	
	if( buildIt != mBuilds.end() ) {
		buildIt->second( CompilationResult( "", "", mErrors, mWarnings, { { "", "" } } ) );
		
		if( mErrors.empty() ) {
			app::console() << "========== rt::Compiler Build succeeded: " << buildIt->first << " ==========" << endl;
		}

		mBuilds.erase( buildIt );
	}
}

}