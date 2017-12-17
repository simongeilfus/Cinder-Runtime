#include "runtime/CompilerMsvc.h"
#include "runtime/Process.h"
#include "runtime/ProjectConfiguration.h"

#include "cinder/app/App.h"
#include "cinder/Xml.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"

using namespace std;
using namespace ci;

namespace runtime {

// Examples: turns 'MyClass' into '??_7MyClass@@6B@', or 'a::b::MyClass' into '??_7MyClass@b@a@@6B@'
// See docs in generateLinkerCommand()
// See MS Doc "Decorated Names": https://msdn.microsoft.com/en-us/library/56h2zst2.aspx?f=255&MSPPError=-2147217396#Format
std::string	CompilerMsvc::getSymbolForVTable( const std::string &typeName ) const
{
	auto parts = ci::split( typeName, "::" );
	string decoratedName;
	for( auto rIt = parts.rbegin(); rIt != parts.rend(); ++rIt ) {
		if( ! rIt->empty() ) // handle leading "::" case, which results in any empty part
			decoratedName += *rIt + "@";
	}

	return "??_7" + decoratedName + "@6B@";
}

std::string CompilerMsvc::printToString() const
{
	stringstream str;
	
	str << "Compiler path: " << getCompilerPath() << " " << getCompilerInitArgs() << endl;
	str << "Subprocess path: " << getCLInitPath() << endl;

	return str.str();
}

void CompilerMsvc::debugLog( BuildSettings *settings ) const
{
	CI_LOG_I( "Compiler Settings: " << Compiler::instance().printToString() );
	CI_LOG_I( "ProjectConfiguration: " << ProjectConfiguration::instance().printToString() );

	if( settings ) {
		CI_LOG_I( "BuildSettings: " << settings->printToString() );
	}
}

CompilerMsvc::CompilerMsvc()
{
	CI_LOG_V( "Tools / Options / Debugging / General / Enable Edit and Continue should be disabled! (And if file locking issues persist try enabling Use Native Compatibility Mode)" );
	
	if( mVerbose ) {
		CI_LOG_I( "Compiler Settings: \n" << printToString() );
	}

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

void CompilerMsvc::build( const std::string &arguments, const std::function<void( const BuildOutput& )> &onBuildFinish )
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

ci::fs::path CompilerMsvc::getCLInitPath() const
{
	return ProjectConfiguration::instance().getProjectDir();
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

std::string CompilerMsvc::generateCompilerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, BuildOutput* output ) const
{
	string command;

	// generate precompile header
	// TODO: This should ideally be handled in a separate build
	if( settings.mCreatePch ) {
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
			
		command += settings.mObjectFilePath.empty() ? "/Fo" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / "/" ).string() + " " : "/Fo" + settings.mObjectFilePath.generic_string() + " ";
		command += "/Fp" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pch" ) ).string() + " ";
	#if defined( _DEBUG )
		command += "/Fd" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pdb" ) ).string() + " ";
	#endif

		command += "/Yc" + settings.getModuleName() + "Pch.h ";

		command += ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + "Pch.cpp" ) ).generic_string();
		command += "\n";
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

	command += settings.mObjectFilePath.empty() ? "/Fo" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / "/" ).string() + " " : "/Fo" + settings.mObjectFilePath.generic_string() + " ";
#if defined( _DEBUG )
	command += settings.mPdbPath.empty() ? "/Fd" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pdb" ) ).string() + " " : "/Fd" + settings.mPdbPath.generic_string() + " ";
#endif
	
	if( settings.mUsePch ) {
		command += "/Fp" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pch" ) ).string() + " ";
		command += "/Yu" + settings.getModuleName() + "Pch.h ";
	}

	// main source file
	command += sourcePath.generic_string() + " ";
	// additional files to compile
	for( const auto &path : settings.mAdditionalSources ) {
		command += path.generic_string() + " ";
		output->getFilePaths().push_back( path );
	}

	return command;
}
std::string CompilerMsvc::generateLinkerCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, BuildOutput* output ) const
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
	
	if( ! settings.mModuleDefPath.empty() ) {
		command += "/DEF:" + settings.mModuleDefPath.string() + " ";
	}
	
	auto outputPath = settings.mOutputPath.empty() ? ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".dll" ) ) : settings.mOutputPath;
	output->setOutputPath( outputPath );
	command += "/OUT:" + outputPath.string() + " ";
#if defined( _DEBUG )
	command += "/DEBUG ";
	//command += "/DEBUG:FASTLINK ";
	command += settings.mPdbPath.empty() ? "/PDB:" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pdb" ) ).string() + " " : "/PDB:" + settings.mPdbPath.generic_string() + " ";
	command += settings.mPdbAltPath.empty() ? "/PDBALTPATH:" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pdb" ) ).string() + " " : "/PDBALTPATH:" + settings.mPdbAltPath.generic_string() + " ";
#endif
	command += "/INCREMENTAL ";
	command += "/DLL ";
	
	// main source file obj
	output->getObjectFilePaths().push_back( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".obj" ) );

	// additional objs to link
	for( const auto &obj : settings.mObjPaths ) {
		command += obj.generic_string() + " ";
		output->getObjectFilePaths().push_back( obj );
	}

	return command;
}

std::string CompilerMsvc::generateBuildCommand( const ci::fs::path &sourcePath, const BuildSettings &settings, BuildOutput* output ) const
{
	auto compilerCommand = generateCompilerCommand( sourcePath, settings, output );
	auto linkerCommand = generateLinkerCommand( sourcePath, settings, output );

	if( settings.isVerboseEnabled() ) {
		CI_LOG_I( "compiler command:\n" << compilerCommand );
		CI_LOG_I( "linker command:\n" << linkerCommand );
	}

	return compilerCommand + linkerCommand;
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


void CompilerMsvc::build( const ci::fs::path &sourcePath, const BuildSettings &settings, const std::function<void(const BuildOutput&)> &onBuildFinish )
{
	if( ! mProcess ) {
		throw CompilerException( "Compiler Process not initialized" );
	}

	// clear the error and warning vectors
	mErrors.clear();
	mWarnings.clear();

	// prepare compilation results
	BuildOutput output;
	output.getFilePaths().push_back( sourcePath );

	auto buildDir = settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build";
	if( ! fs::exists( buildDir ) ) {
		fs::create_directories( buildDir );
	}

#if defined( _DEBUG ) && 1
	// try renaming previous pdb files to prevent errors
	auto pdb = settings.mPdbPath.empty() ? ( buildDir / ( settings.getModuleName() + ".pdb" ) ) : settings.mPdbPath;
	/*if( fs::exists( pdb ) ) {
		auto newName = getNextAvailableName( pdb );
		try {
			fs::rename( pdb, newName );
		} catch( const std::exception & ) {}
	}*/
	output.setPdbFilePath( pdb );
#endif

	// execute pre build steps
	auto buildSettings = settings;
	for( const auto &buildStep : settings.mPreBuildSteps ) {
		buildStep->execute( &buildSettings );
	}
		
	// issue the build command with a completion token
	auto command = generateBuildCommand( sourcePath, buildSettings, &output );
	output.setBuildSettings( buildSettings );
	mBuilds.insert( { sourcePath.filename(), { output, onBuildFinish } } );
	app::console() << endl << "1>------ Runtime Compiler Build started: Project: " << ProjectConfiguration::instance().getProjectPath().stem() << ", Configuration: " << ProjectConfiguration::instance().getConfiguration() << " " << ProjectConfiguration::instance().getPlatform() << " ------" << endl;
	app::console() << "1>  " << sourcePath.filename() << endl;
	mProcess << command << endl << ( "CI_BUILD " + sourcePath.filename().string() ) << endl;
}

void CompilerMsvc::build( const std::vector<ci::fs::path> &sourcesPaths, const BuildSettings &settings, const std::function<void( const BuildOutput& )> &onBuildFinish )
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
	auto it = s.find( ProjectConfiguration::instance().getProjectDir().generic_string() );
	if( it != std::string::npos ) {
		return s.substr( ProjectConfiguration::instance().getProjectDir().generic_string().length() );
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
			//mErrors.push_back( trimProjectDir( output ) );	
			mErrors.push_back( output );	
		}
		else if( output.find( "warning" ) != string::npos ) {
			// mWarnings.push_back( trimProjectDir( output ) );
			mWarnings.push_back( output );
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

			// execute post build steps
			const Build &build = buildIt->second;
			BuildOutput buildOutput = build.first;
			for( const auto &buildStep : buildOutput.getBuildSettings().mPostBuildSteps ) {
				buildStep->execute( &buildOutput );
			}

			// print results
			app::console() << "1>  " << buildOutput.getFilePaths().front().filename() << " -> " << buildOutput.getOutputPath() << endl;
			if( ! buildOutput.getPdbFilePath().empty() ) {
				app::console() << "1>  " << buildOutput.getFilePaths().front().filename() << " -> " << buildOutput.getPdbFilePath() << endl;
			}
			app::console() << "========== Runtime Compiler Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========" << endl;
			auto elapsed = std::chrono::system_clock::now() - buildOutput.getTimePoint();
			auto elapsedMicro = std::chrono::duration_cast<std::chrono::microseconds>( elapsed ).count();
			auto elapsedMinutes = std::chrono::duration_cast<std::chrono::hours>( elapsed ).count();
			auto elapsedHours = std::chrono::duration_cast<std::chrono::hours>( elapsed ).count();
			std::ostringstream oss;
			oss << std::setfill('0') << std::setw(2) << elapsedHours << ":" << std::setw(2) << elapsedMinutes << ":"
				<< std::setw(2) << ( elapsedMicro % 1000000000 ) / 1000000 << "." << std::setw(3) << ( elapsedMicro % 1000000 ) / 1000;
			app::console() << endl << "Time Elapsed " << oss.str() << endl << endl;

			// call the build finish callback
			build.second( buildOutput );
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