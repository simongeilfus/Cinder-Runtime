#include "runtime/CompilerMsvc.h"
#include "runtime/ClassFactory.h"
#include "runtime/PrecompiledHeader.h"
#include "runtime/Process.h"

#include "cinder/app/App.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"

using namespace std;
using namespace ci;

namespace runtime {

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::default( )
{
	return include( CI_RT_PROJECT_ROOT / "include" )
		.include( CI_RT_PROJECT_ROOT / "src" )
		.include( CI_RT_CINDER_PATH / "include" )
		.libraryPath( CI_RT_CINDER_PATH / "lib/msw" / CI_RT_PLATFORM_TARGET )
		.libraryPath( CI_RT_CINDER_PATH / "lib/msw" / CI_RT_PLATFORM_TARGET / CI_RT_CONFIGURATION / CI_RT_PLATFORM_TOOLSET )
		.library( "cinder.lib" )
		.define( "CINDER_SHARED" ).define( "WIN32" ).define( "_WIN32_WINNT=0x0601" ).define( "_WINDOWS" ).define( "NOMINMAX" ).define( "_UNICODE" ).define( "UNICODE" )
		.compilerOption( "/nologo" ).compilerOption( "/W3" ).compilerOption( "/WX-" ).compilerOption( "/EHsc" ).compilerOption( "/RTC1" ).compilerOption( "/GS" )
		.compilerOption( "/fp:precise" ).compilerOption( "/Zc:wchar_t" ).compilerOption( "/Zc:forScope" ).compilerOption( "/Zc:inline" ).compilerOption( "/Gd" ).compilerOption( "/TP" )
		//.compilerOption( "/Gm" )
		
#if defined( _DEBUG )
		.compilerOption( "/Od" )
		.compilerOption( "/Zi" )
		.define( "_DEBUG" )
		.compilerOption( "/MDd" )
#else
		.compilerOption( "/MD" )
#endif
		//.linkerOption( "/INCREMENTAL:NO" )
		.linkerOption( "/NOLOGO" ).linkerOption( "/NODEFAULTLIB:LIBCMT" ).linkerOption( "/NODEFAULTLIB:LIBCPMT" )
		.define( "RT_COMPILED" )

		// cinder-runtime include 
		.include( fs::absolute(  fs::path( __FILE__ ).parent_path().parent_path().parent_path() / "include" ) )
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

CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::usePrecompiledHeader( bool use /*const ci::fs::path &path*/ )
{
	mUsePch = use;
	//mPrecompiledHeader = path;
	return *this;
}
		
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::createPrecompiledHeader( bool create /*const ci::fs::path &path*/ )
{
	mGeneratePch = create;
	//mPrecompiledHeader = path;
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
: mLinkAppObjs( true ), mGenerateFactory( true ), mGeneratePch( false ), mUsePch( true )
{
}

CompilerMsvc::CompilerMsvc()
{
	CI_LOG_V( "Tools / Options / Debugging / General / Enable Edit and Continue should be disabled! (And if file locking issues persist try enabling Use Native Compatibility Mode)" );

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
#if _MSC_VER == 1900
	return "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat";
#elif _MSC_VER >= 1910
	return "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat";
#else
	#error Visual Studio version not supported.
#endif
}

std::string CompilerMsvc::getCompilerInitArgs() const
{
#ifdef _WIN64
	return " x64";
#else
	return " x86";
#endif
}

std::string CompilerMsvc::generateCompilerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const
{
	string command;

	// generate precompile header
	if( settings.mUsePch ) {
		
		bool createPch = generatePrecompiledHeader( sourcePath, 
			CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + "Pch.h" ),
			CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + "Pch.cpp" ), false ) || settings.mGeneratePch;

		if( createPch ) {
			command += "cl /c ";

			for( const auto &define : settings.mPpDefinitions ) {
				command += "/D " + define + " ";
			}
			for( const auto &include : settings.mIncludes ) {
				command += "/I" + include.generic_string() + " ";
			}
			for( const auto &include : settings.mForcedIncludes ) {
				command += "/FI " + include + " ";
			}
			for( const auto &compilerArg : settings.mCompilerOptions ) {
				command += compilerArg + " ";
			}
			
			command += settings.mObjectFilePath.empty() ? "/Fo" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / "/" ).string() + " " : "/Fo" + settings.mObjectFilePath.generic_string() + " ";
			command += "/Fp" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + ".pch" ) ).string() + " ";
		#if defined( _DEBUG )
			command += "/Fd" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / "/" ).string() + " ";
		#endif

			command += "/Yc" + sourcePath.stem().string() + "Pch.h ";

			command += ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + "Pch.cpp" ) ).generic_string();
			command += "\n";
		}
	}

	command += "cl ";
	command += "/MP ";
	
	for( const auto &define : settings.mPpDefinitions ) {
		command += "/D " + define + " ";
	}
	for( const auto &include : settings.mIncludes ) {
		command += "/I" + include.generic_string() + " ";
	}
	for( const auto &include : settings.mForcedIncludes ) {
		command += "/FI" + include + " ";
	}
	for( const auto &compilerArg : settings.mCompilerOptions ) {
		command += compilerArg + " ";
	}

	command += settings.mObjectFilePath.empty() ? "/Fo" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / "/" ).string() + " " : "/Fo" + settings.mObjectFilePath.generic_string() + " ";
#if defined( _DEBUG )
	command += settings.mPdbPath.empty() ? "/Fd" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / "/" ).string() + " " : "/Fd" + settings.mPdbPath.generic_string() + " ";
#endif
	
	if( settings.mUsePch ) {
		command += "/Fp" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + ".pch" ) ).string() + " ";
		command += "/Yu" + sourcePath.stem().string() + "Pch.h ";
		command += "/FI" + sourcePath.stem().string() + "Pch.h ";
	}

	// main source file
	command += sourcePath.generic_string() + " ";
	// additional files to compile
	for( const auto &path : settings.mAdditionalSources ) {
		command += path.generic_string() + " ";
		result->getFilePaths().push_back( path );
	}

	return command;
}
std::string CompilerMsvc::generateLinkerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const
{
	string command = "/link ";
	
	for( const auto &libraryPath : settings.mLibraryPaths ) {
		command += "/LIBPATH:" + libraryPath.generic_string() + " ";
	}
	for( const auto &library : settings.mLibraries ) {
		command += library + " ";
	}
	for( const auto &linkerArg : settings.mLinkerOptions ) {
		command += linkerArg + " ";
	}
	
	// TODO: Make this optional
	// vtable symbol export
	// https://social.msdn.microsoft.com/Forums/vstudio/en-US/0cb15e28-4852-4cba-b63d-8a0de6e88d5f/accessing-the-vftable-vfptr-without-constructing-the-object?forum=vclanguage
	// https://www.gamedev.net/forums/topic/392971-c-compile-time-retrival-of-a-classs-vtable-solved/?page=2
	// https://www.gamedev.net/forums/topic/460569-c-compile-time-retrival-of-a-classs-vtable-solution-2/
	if( ! fs::exists( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + ".def" ) ) ) {
		// create a .def file with the symbol of the vtable to be able to find it with GetProcAddress	
		std::ofstream outputFile( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + ".def" ) );		
		outputFile << "EXPORTS" << endl;
		outputFile << "\t??_7" << sourcePath.stem() << "@@6B@\t\tDATA" << endl;
	}
	command += "/DEF:" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + ".def" ) ).string() + " ";
	
	auto outputPath = settings.mOutputPath.empty() ? ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + ".dll" ) ) : settings.mOutputPath;
	result->setOutputPath( outputPath );
	command += "/OUT:" + outputPath.string() + " ";
#if defined( _DEBUG )
	command += "/DEBUG:FASTLINK ";
	command += settings.mPdbPath.empty() ? "/PDB:" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + ".pdb" ) ).string() + " " : "/PDB:" + settings.mPdbPath.generic_string() + " ";
	command += settings.mPdbPath.empty() ? "/PDBALTPATH:" + ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + ".pdb" ) ).string() + " " : "/PDBALTPATH:" + settings.mPdbPath.generic_string() + " ";
#endif
	command += "/INCREMENTAL ";
	command += "/DLL ";

	result->getObjectFilePaths().push_back( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + ".obj" ) );
	for( const auto &obj : settings.mObjPaths ) {
		command += obj.generic_string() + " ";
		result->getObjectFilePaths().push_back( obj );
	}
	
	command += ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + "Pch.obj" ) ).generic_string() + " ";
	command += ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + "Factory.obj" ) ).generic_string() + " ";

	if( settings.mLinkAppObjs ) {
		for( auto it = fs::directory_iterator( CI_RT_INTERMEDIATE_DIR ), end = fs::directory_iterator(); it != end; it++ ) {
			if( it->path().extension() == ".obj" ) {
				// Skip obj for current source and current app
				if( it->path().filename().string().find( sourcePath.stem().string() + ".obj" ) == string::npos 
					&& it->path().filename().string().find( CI_RT_PROJECT_PATH.stem().string() + "App.obj" ) == string::npos
					) {
					command += it->path().generic_string() + " ";
					result->getObjectFilePaths().push_back( it->path() );
				}
			}
		}
	}

	return command;
}

std::string CompilerMsvc::generateBuildCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, CompilationResult* result ) const
{
	return generateCompilerCommand( sourcePath, settings, result ) + generateLinkerCommand( sourcePath, settings, result );
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
	
    std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now();

	// clear the error and warning vectors
	mErrors.clear();
	mWarnings.clear();

	// prepare compilation results
	CompilationResult result;
	result.getFilePaths().push_back( sourcePath );

	// make sure the intermediate directories exists
	if( ! fs::exists( CI_RT_INTERMEDIATE_DIR / "runtime" ) ) {
		fs::create_directory( CI_RT_INTERMEDIATE_DIR / "runtime" );
	}
	if( ! fs::exists( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() ) ) {
		fs::create_directory( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() );
	}
	if( ! fs::exists( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" ) ) {
		fs::create_directory( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" );
	}

#if defined( _DEBUG ) && 1
	// try renaming previous pdb files to prevent errors
	auto pdb = settings.mPdbPath.empty() ? ( CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + ".pdb" ) ) : settings.mPdbPath;
	if( fs::exists( pdb ) ) {
		auto newName = getNextAvailableName( pdb );
		try {
			fs::rename( pdb, newName );
		} catch( const std::exception & ) {}
	}
	result.setPdbFilePath( pdb );
#endif

	// generate factor if needed and add it to the compiler list
	auto buildSettings = settings;
	if( settings.mGenerateFactory ) {
		auto factoryPath = CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / ( sourcePath.stem().string() + "Factory.cpp" );
		if( ! fs::exists( factoryPath ) ) {
			generateClassFactory( factoryPath, sourcePath.stem().string() );
		}
		auto factoryObjPath = CI_RT_INTERMEDIATE_DIR / "runtime" / sourcePath.stem() / "build" / ( sourcePath.stem().string() + "Factory.obj" );
		//if( ! fs::exists( factoryObjPath ) ) {
			buildSettings.additionalSource( factoryPath );
		//}
		//else {
		//	buildSettings.linkObj( factoryObjPath );
		//}
	}
		
	// issue the build command with a completion token
	auto command = generateBuildCommand( sourcePath, buildSettings, &result );
	mBuilds.insert( make_pair( sourcePath.filename(), make_tuple( result, onBuildFinish, timePoint ) ) );
	app::console() << endl << "1>------ Runtime Compiler Build started: Project: " << CI_RT_PROJECT_PATH.stem() << ", Configuration: " << CI_RT_CONFIGURATION << " " << CI_RT_PLATFORM_TARGET << " ------" << endl;
	app::console() << "1>  " << sourcePath.filename() << endl;
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

namespace {
std::string trimProjectDir( const std::string &s )
{
	auto it = s.find( CI_RT_PROJECT_DIR.generic_string() );
	if( it != std::string::npos ) {
		return s.substr( CI_RT_PROJECT_DIR.generic_string().length() );
	}
	else {
		return s;
	}
}
}

void CompilerMsvc::parseProcessOutput()
{
	string fullOutput;
	auto buildIt = mBuilds.end();
	while( mProcess->isOutputAvailable() ) {
		auto output = removeEndline( mProcess->getOutputAsync() );
		if( output.find( "error" ) != string::npos ) { 
			mErrors.push_back( trimProjectDir( output ) );	
		}
		else if( output.find( "warning" ) != string::npos ) {
			mWarnings.push_back( trimProjectDir( output ) );
		}
		if( output.find( "CI_BUILD" ) != string::npos ) {
			buildIt = mBuilds.find( output.substr( output.find_first_of( " " ) + 1 ) );
		}
		fullOutput += output;
	}
	
	if( mVerbose && ! fullOutput.empty() ) app::console() << fullOutput << endl;
	
	if( buildIt != mBuilds.end() ) {
		
		for( auto warning : mWarnings ) {
			app::console() << "1>" + warning << endl;
		}	
		if( mErrors.empty() ) {
			const Build &build = buildIt->second;
			app::console() << "1>  " << std::get<0>( build ).getFilePaths().front().filename() << " -> " << std::get<0>( build ).getOutputPath() << endl;
			if( ! std::get<0>( build ).getPdbFilePath().empty() ) {
				app::console() << "1>  " << std::get<0>( build ).getFilePaths().front().filename() << " -> " << std::get<0>( build ).getPdbFilePath() << endl;
			}
			app::console() << "========== Runtime Compiler Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========" << endl;
			auto elapsed = std::chrono::steady_clock::now() - std::get<2>( build );
			auto elapsedMicro = std::chrono::duration_cast<std::chrono::microseconds>( elapsed ).count();
			auto elapsedMinutes = std::chrono::duration_cast<std::chrono::hours>( elapsed ).count();
			auto elapsedHours = std::chrono::duration_cast<std::chrono::hours>( elapsed ).count();
			std::ostringstream oss;
			oss << std::setfill('0') << std::setw(2) << elapsedHours << ":" << std::setw(2) << elapsedMinutes << ":"
				<< std::setw(2) << ( elapsedMicro % 1000000000 ) / 1000000 << "." << std::setw(3) << ( elapsedMicro % 1000000 ) / 1000;
			app::console() << endl << "Time Elapsed " << oss.str() << endl << endl;
			std::get<1>( build )( std::get<0>( build ) );
		}
		else {
			for( auto error : mErrors ) {
				app::console() << "1>" + error << endl;
			}
			app::console() << "========== Runtime Compiler Build: 0 succeeded, 1 failed, 0 up-to-date, 0 skipped ==========" << endl;
		}

		mBuilds.erase( buildIt );
	}
}

}