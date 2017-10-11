#include "runtime/CompilerMsvc.h"
#include "runtime/ClassFactory.h"
#include "runtime/PrecompiledHeader.h"
#include "runtime/Process.h"

#include "cinder/app/App.h"
#include "cinder/Xml.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"

#define RT_VERBOSE_DEFAULT 1

using namespace std;
using namespace ci;

namespace runtime {

namespace {

	struct ProjectConfiguration {
		ProjectConfiguration()
		{
			fs::path appPath = app::getAppPath();
			const size_t maxDepth = 20;
			size_t depth = 0;
			for( fs::path path = appPath; path.has_parent_path() || ( path == appPath ); path = path.parent_path(), ++depth ) {
				if( depth >= maxDepth || ! projectPath.empty() )
					break;

				for( fs::directory_iterator it = fs::directory_iterator( path ), end; it != end; ++it ) {
					if( it->path().extension() == ".vcxproj" ) {
						projectPath = it->path();
						break;
					}
				}
			}

			if( projectPath.empty() ) {
				string msg = "Failed to find the .vcxproj path for this executable.";
				msg += " Searched up " + to_string( maxDepth ) + " levels from app path: " + appPath.string();
				throw CompilerException( msg );
			}

			projectDir = projectPath.parent_path();

	#ifdef _WIN64
			platform = "x64";
			platformTarget = "x64";
	#else
			platform = "Win32";
			platformTarget = "x86";
	#endif

	#if defined( _DEBUG )
			configuration = "Debug";
	#else
			configuration = "Release";
	#endif

	#if defined( CINDER_SHARED )
			configuration += "_Shared";
	#endif
		
	#if _MSC_VER == 1900
			platformToolset = "v140";
	#elif _MSC_VER >= 1910
			platformToolset = "v141";
	#else
			platformToolset = "v120";
	#endif

		}

		string printToString() const
		{
			stringstream str;

			str << "projectPath: " << projectPath
				<< "\n\t- configuration: " << configuration << ", platform: " << platform << ", platformTarget: " << platformTarget << ", platformToolset: " << platformToolset;

			return str.str();
		}

		string configuration;
		string platform;
		string platformTarget;
		string platformToolset;
		fs::path projectPath;
		fs::path projectDir;
	};

	ProjectConfiguration& getProjectConfiguration()
	{
		static ProjectConfiguration config;
		return config;
	}

	// http://stackoverflow.com/questions/5343190/how-do-i-replace-all-instances-of-a-string-with-another-string
	void replaceAll( string& str, const string& from, const string& to ) 
	{
		if(from.empty())
			return;
		string wsRet;
		wsRet.reserve(str.length());
		size_t start_pos = 0, pos;
		while((pos = str.find(from, start_pos)) != string::npos) {
			wsRet += str.substr(start_pos, pos - start_pos);
			wsRet += to;
			pos += from.length();
			start_pos = pos;
		}
		wsRet += str.substr(start_pos);
		str.swap(wsRet); // faster than str = wsRet;
	}

	bool matchCondition( const std::string &condition, const ProjectConfiguration &config ) 
	{
		if( condition.find( "==" ) == string::npos || condition.find( "$(Configuration)" ) == string::npos || condition.find( "$(Platform)" ) == string::npos ) {
			return false;
		}
		// extract left and right part of the condition
		string conditionLhs = condition.substr( 1, condition.find_last_of( "==" ) - 3 );
		string conditionRhs = condition.substr( condition.find_last_of( "==" ) + 2 );
		conditionRhs = conditionRhs.substr( 0, conditionRhs.length() - 1 );
		// replace macros
		replaceAll( conditionLhs, "$(Configuration)", config.configuration );
		replaceAll( conditionLhs, "$(Platform)", config.platform );
		return ( conditionLhs == conditionRhs );
	};

	string replaceVcxprojMacros( const std::string &input, const ProjectConfiguration &config )
	{
		string output = input;
		replaceAll( output, "$(Configuration)", config.configuration );
		replaceAll( output, "$(Platform)", config.platform );
		replaceAll( output, "$(PlatformTarget)", config.platformTarget );
		replaceAll( output, "$(PlatformToolset)", config.platformToolset );
		replaceAll( output, "$(ProjectDir)", config.projectDir.string() + "/" );
		return output;
	}

	void parsePropertySheet( CompilerMsvc::BuildSettings* settings, const fs::path &fullPath )
	{
		if( ! fs::exists( fullPath ) ) {
			CI_LOG_E( "expected property sheet doesn't exist at: " << fullPath << ", skipping." );
			return;
		}

		auto propSheet = XmlTree( loadFile( fullPath ) );
		const auto &projectNode = propSheet.getChild( "Project" );

		for( const auto &child : projectNode.getChildren() ) {			
			if( child->getTag() != "PropertyGroup" )
				continue;

			if( child->hasAttribute( "Label" ) ) {
				// skip base template prop sheet
				if( child->getAttributeValue<string>( "Label" ) == "UserMacros" ) {
					for( const auto &macroNode : child->getChildren() ) {
						auto name = "$(" + macroNode->getTag() + ")";
						auto value = macroNode->getValue<string>();
						settings->userMacro( name, value );
					}
				}
			}
		}
	}

	void parseVcxproj( CompilerMsvc::BuildSettings* settings, const XmlTree &node, const ProjectConfiguration &config, bool matched = false )
	{
		if( ! matched && node.hasAttribute( "Condition" ) ) {
			matched = matchCondition( node.getAttributeValue<string>( "Condition" ), config );

			if( ! matched ) {
				return;
			}
		}

		if( node.getTag() == "OutDir" ) {
			auto outDir = fs::path( replaceVcxprojMacros( node.getValue<string>(), config ) );
//			settings->outputPath( outDir.parent_path() );
		}
		else if( node.getTag() == "IntDir" ) {
			auto intDir = fs::path( replaceVcxprojMacros( node.getValue<string>(), config ) );
			settings->intermediatePath( intDir.parent_path() );
		}
		else if( node.getTag() == "LinkIncremental" ) {
			//console() << "LinkIncremental = " << node.getValue<string>() << endl;
		}
		else if( node.getTag() == "AdditionalIncludeDirectories" ) {
			vector<string> includes = ci::split( replaceVcxprojMacros( node.getValue<string>(), config ), ";" );
			for( const auto &inc : includes ) {
				if( ! inc.empty() ) {
					settings->include( fs::path( inc ) );
				}
			}
		}
		else if( node.getTag() == "PreprocessorDefinitions" ) {
			string definitionsString = node.getValue<string>();
			replaceAll( definitionsString, "%(PreprocessorDefinitions)", "" );
			vector<string> definitions = ci::split( definitionsString, ";" );
			for( const auto &def : definitions ) {
				if( ! def.empty() ) {
					settings->define( def );
				}
			}
		}

		else if( node.getTag() == "AdditionalDependencies" ) {
			string librariesString = node.getValue<string>();
			replaceAll( librariesString, "%(AdditionalDependencies)", "" );
			vector<string> libraries = ci::split( librariesString, ";" );
			for( const auto &lib : libraries ) {
				if( ! lib.empty() ) {
					settings->library( lib );
				}
			}
		}
		else if( node.getTag() == "AdditionalLibraryDirectories" ) {
			vector<string> libraryDirectories = ci::split( replaceVcxprojMacros( node.getValue<string>(), config ), ";" );
			for( auto dir : libraryDirectories ) {
				if( ! dir.empty() ) {
					settings->libraryPath( fs::path( dir ) );
				}
			}
		}
		else if( node.getTag() == "ImportGroup" ) {
			// parse user property sheets
			// TODO: document limitations
			if( node.getAttributeValue<string>( "Label" ) == "PropertySheets" ) {
				for( const auto &child : node.getChildren() ) {
					if( child->hasAttribute( "Label" ) ) {
						// skip base template prop sheet
						if( child->getAttributeValue<string>( "Label" ) == "LocalAppDataPlatform")
							continue;
					}

					string fileName = child->getAttributeValue<string>( "Project" );
					fs::path propSheetFullPath = config.projectDir / fileName;

					CI_LOG_I( "Parsing property sheet at: " << propSheetFullPath );
					parsePropertySheet( settings, propSheetFullPath );
				}
			}
		}
		// skip
		else if( node.getTag() == "ResourceCompile" ) {
			return;
		}
			
		for( const auto &child : node.getChildren() ) {
			parseVcxproj( settings, *child, config, matched );
		}
	}
}

/*CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::default( )
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
}*/

CompilerMsvc::BuildSettings::BuildSettings()
: mLinkAppObjs( true ), mGenerateFactory( true ), mGeneratePch( false ), mUsePch( true ), mConfiguration( getProjectConfiguration().configuration ), mPlatform( getProjectConfiguration().platform ), mPlatformToolset( getProjectConfiguration().platformToolset )
{
}

CompilerMsvc::BuildSettings::BuildSettings( bool defaultSettings )
: mLinkAppObjs( true ), mGenerateFactory( true ), mGeneratePch( false ), mUsePch( true ), mConfiguration( getProjectConfiguration().configuration ), mPlatform( getProjectConfiguration().platform ), mPlatformToolset( getProjectConfiguration().platformToolset )
{
	compilerOption( "/nologo" ).compilerOption( "/W3" ).compilerOption( "/WX-" ).compilerOption( "/EHsc" ).compilerOption( "/RTC1" ).compilerOption( "/GS" )
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
	// app src folder
	.include( "../src" )
	.verbose( RT_VERBOSE_DEFAULT )
	;

	if( defaultSettings ) {
		parseVcxproj( this, XmlTree( loadFile( getProjectConfiguration().projectPath ) ), getProjectConfiguration() );
	}

	if( mVerbose ) {
		CI_LOG_I( "ProjectConfiguration: " << getProjectConfiguration().printToString() );
		CI_LOG_I( "BuildSettings: " << printToString() );
	}
}

CompilerMsvc::BuildSettings::BuildSettings( const ci::fs::path &vcxProjPath )
: mLinkAppObjs( true ), mGenerateFactory( true ), mGeneratePch( false ), mUsePch( true ), mConfiguration( getProjectConfiguration().configuration ), mPlatform( getProjectConfiguration().platform ), mPlatformToolset( getProjectConfiguration().platformToolset )
{
	getProjectConfiguration().projectPath = vcxProjPath;
	getProjectConfiguration().projectDir = vcxProjPath.parent_path();
	
	compilerOption( "/nologo" ).compilerOption( "/W3" ).compilerOption( "/WX-" ).compilerOption( "/EHsc" ).compilerOption( "/RTC1" ).compilerOption( "/GS" )
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
	.verbose( RT_VERBOSE_DEFAULT )

	// cinder-runtime include 
	.include( fs::absolute(  fs::path( __FILE__ ).parent_path().parent_path().parent_path() / "include" ) );

	parseVcxproj( this, XmlTree( loadFile( getProjectConfiguration().projectPath ) ), getProjectConfiguration() );

	if( mVerbose ) {
		CI_LOG_I( "ProjectConfiguration: " << getProjectConfiguration().printToString() );
		CI_LOG_I( "BuildSettings: " << printToString() );
	}
}

std::string CompilerMsvc::BuildSettings::printToString() const
{
	stringstream str;

	str << "link app objs: " << mLinkAppObjs << ", generate factory: " << mGenerateFactory << ", generate pch: " << mGeneratePch << ", use pch: " << mUsePch << "\n";
	str << "precompiled header: " << mPrecompiledHeader << "\n";
	str << "output path: " << mOutputPath << "\n";
	str << "intermediate path: " << mIntermediatePath << "\n";
	str << "pdb path: " << mPdbPath << "\n";
	str << "module name: " << mModuleName << "\n";
	str << "includes:\n";
	for( const auto &include : mIncludes ) {
		str << "\t- " << include << "\n";
	}
	str << "library paths:\n";
	for( const auto &path : mLibraryPaths ) {
		str << "\t- " << path << "\n";
	}
	str << "libraries:\n";
	for( const auto &lib : mLibraries ) {
		str << "\t- " << lib << "\n";
	}
	str << "additional sources:\n";
	for( const auto &src : mAdditionalSources ) {
		str << "\t- " << src << "\n";
	}
	str << "forced includes:\n";
	for( const auto &include : mForcedIncludes ) {
		str << "\t- " << include << "\n";
	}
	str << "preprocessor definitions: ";
	for( const auto &ppDefine : mPpDefinitions ) {
		str << ppDefine << " ";
	}
	str << endl;
	str << "compiler options: ";
	for( const auto &flag : mCompilerOptions ) {
		str << flag << " ";
	}
	str << endl;
	str << "linker options: ";
	for( const auto &flag : mLinkerOptions ) {
		str << flag << " ";
	}
	str << endl;
	str << "user macros:\n";
	for( const auto &macro : mUserMacros ) {
		str << "\t- " << macro.first << " = " << macro.second << "\n";
	}
	str << endl;

	return str.str();
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
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::userMacro( const std::string &name, const std::string &value )
{
	mUserMacros[name] = value;
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
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::intermediatePath( const ci::fs::path &path )
{
	mIntermediatePath = path;
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::configuration( const std::string &option )
{
	mConfiguration = option;
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::platform( const std::string &option )
{
	mPlatform = option;
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::platformToolset( const std::string &option )
{
	mPlatformToolset = option;
	return *this;
}
CompilerMsvc::BuildSettings& CompilerMsvc::BuildSettings::moduleName( const std::string &name )
{
	mModuleName = name;
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

ci::fs::path CompilerMsvc::getCLInitPath() const
{
	return getProjectConfiguration().projectDir;
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
			settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + "Pch.h" ),
			settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + "Pch.cpp" ), false ) || settings.mGeneratePch;

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
			
			command += settings.mObjectFilePath.empty() ? "/Fo" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / "/" ).string() + " " : "/Fo" + settings.mObjectFilePath.generic_string() + " ";
			command += "/Fp" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pch" ) ).string() + " ";
		#if defined( _DEBUG )
			command += "/Fd" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / "/" ).string() + " ";
		#endif

			command += "/Yc" + settings.getModuleName() + "Pch.h ";

			command += ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + "Pch.cpp" ) ).generic_string();
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

	command += settings.mObjectFilePath.empty() ? "/Fo" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / "/" ).string() + " " : "/Fo" + settings.mObjectFilePath.generic_string() + " ";
#if defined( _DEBUG )
	command += settings.mPdbPath.empty() ? "/Fd" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / "/" ).string() + " " : "/Fd" + settings.mPdbPath.generic_string() + " ";
#endif
	
	if( settings.mUsePch ) {
		command += "/Fp" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pch" ) ).string() + " ";
		command += "/Yu" + settings.getModuleName() + "Pch.h ";
		command += "/FI" + settings.getModuleName() + "Pch.h ";
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
	if( ! fs::exists( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + ".def" ) ) ) {
		// create a .def file with the symbol of the vtable to be able to find it with GetProcAddress	
		std::ofstream outputFile( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + ".def" ) );		
		outputFile << "EXPORTS" << endl;
		outputFile << "\t??_7" << settings.getModuleName() << "@@6B@\t\tDATA" << endl;
	}
	command += "/DEF:" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + ".def" ) ).string() + " ";
	
	auto outputPath = settings.mOutputPath.empty() ? ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".dll" ) ) : settings.mOutputPath;
	result->setOutputPath( outputPath );
	command += "/OUT:" + outputPath.string() + " ";
#if defined( _DEBUG )
	command += "/DEBUG:FASTLINK ";
	command += settings.mPdbPath.empty() ? "/PDB:" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pdb" ) ).string() + " " : "/PDB:" + settings.mPdbPath.generic_string() + " ";
	command += settings.mPdbPath.empty() ? "/PDBALTPATH:" + ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pdb" ) ).string() + " " : "/PDBALTPATH:" + settings.mPdbPath.generic_string() + " ";
#endif
	command += "/INCREMENTAL ";
	command += "/DLL ";

	result->getObjectFilePaths().push_back( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".obj" ) );
	for( const auto &obj : settings.mObjPaths ) {
		command += obj.generic_string() + " ";
		result->getObjectFilePaths().push_back( obj );
	}
	
	command += ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + "Pch.obj" ) ).generic_string() + " ";

	if( settings.mGenerateFactory ) { 
		command += ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + "Factory.obj" ) ).generic_string() + " ";
	}

	if( settings.mLinkAppObjs ) {
		for( auto it = fs::directory_iterator( settings.getIntermediatePath() ), end = fs::directory_iterator(); it != end; it++ ) {
			if( it->path().extension() == ".obj" ) {
				// Skip obj for current source and current app
				if( it->path().filename().string().find( settings.getModuleName() + ".obj" ) == string::npos 
					&& it->path().filename().string().find( getProjectConfiguration().projectPath.stem().string() + "App.obj" ) == string::npos
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
	if( ! fs::exists( settings.getIntermediatePath() / "runtime" ) ) {
		fs::create_directory( settings.getIntermediatePath() / "runtime" );
	}
	if( ! fs::exists( settings.getIntermediatePath() / "runtime" / settings.getModuleName() ) ) {
		fs::create_directory( settings.getIntermediatePath() / "runtime" / settings.getModuleName() );
	}
	if( ! fs::exists( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" ) ) {
		fs::create_directory( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" );
	}

#if defined( _DEBUG ) && 1
	// try renaming previous pdb files to prevent errors
	auto pdb = settings.mPdbPath.empty() ? ( settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + ".pdb" ) ) : settings.mPdbPath;
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
		auto factoryPath = settings.getIntermediatePath() / "runtime" / settings.getModuleName() / ( settings.getModuleName() + "Factory.cpp" );
		if( ! fs::exists( factoryPath ) ) {
			generateClassFactory( factoryPath, settings.getModuleName() );
		}
		auto factoryObjPath = settings.getIntermediatePath() / "runtime" / settings.getModuleName() / "build" / ( settings.getModuleName() + "Factory.obj" );
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
	app::console() << endl << "1>------ Runtime Compiler Build started: Project: " << getProjectConfiguration().projectPath.stem() << ", Configuration: " << getProjectConfiguration().configuration << " " << getProjectConfiguration().platform << " ------" << endl;
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
	auto it = s.find( getProjectConfiguration().projectDir.generic_string() );
	if( it != std::string::npos ) {
		return s.substr( getProjectConfiguration().projectDir.generic_string().length() );
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