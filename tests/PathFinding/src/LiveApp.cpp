#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"

#include <regex>

#include "runtime/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LiveApp : public App {
public:
	LiveApp();
	void setup() override;
	void draw() override;
};

LiveApp::LiveApp()
{
}

struct ProjectConfiguration {
	ProjectConfiguration()
	{
		fs::path appPath = getAppPath();
		size_t depth = 0;
		for( fs::path path = appPath; path.has_parent_path() || ( path == appPath ); path = path.parent_path(), ++depth ) {
			if( depth >= 5 || ! projectPath.empty() )
				break;

			for( fs::directory_iterator it = fs::directory_iterator( path ), end; it != end; ++it ) {
				if( it->path().extension() == ".vcxproj" ) {
					projectPath = it->path();
					break;
				}
			}
		}	

		projectDir = projectPath.parent_path();

#ifdef _WIN64
		platform = "x64";
#else
		platform = "Win32";
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

	string configuration;
	string platform;
	string platformToolset;
	fs::path projectPath;
	fs::path projectDir;
};

namespace {
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
		replaceAll( output, "$(PlatformTarget)", config.platform );
		replaceAll( output, "$(PlatformToolset)", config.platformToolset );
		replaceAll( output, "$(ProjectDir)", config.projectDir.string() );
		return output;
	}

	void parseVcxproj( const XmlTree &node, const ProjectConfiguration &config, bool matched = false )
	{
		if( ! matched && node.hasAttribute( "Condition" ) ) {
			matched = matchCondition( node.getAttributeValue<string>( "Condition" ), config );

			if( ! matched ) {
				return;
			}
		}

		if( node.getTag() == "OutDir" ) {
			console() << "$(OutDir) = " << replaceVcxprojMacros( node.getValue<string>(), config ) << endl;
		}
		else if( node.getTag() == "IntDir" ) {
			console() << "$(IntDir) = " << replaceVcxprojMacros( node.getValue<string>(), config ) << endl;
		}
		else if( node.getTag() == "LinkIncremental" ) {
			console() << "LinkIncremental = " << node.getValue<string>() << endl;
		}
		else if( node.getTag() == "AdditionalIncludeDirectories" ) {
			console() << "AdditionalIncludeDirectories = " << replaceVcxprojMacros( node.getValue<string>(), config ) << endl;
		}
		else if( node.getTag() == "PreprocessorDefinitions" ) {
			console() << "PreprocessorDefinitions = " << node.getValue<string>() << endl;
		}
		else if( node.getTag() == "AdditionalDependencies" ) {
			console() << "AdditionalDependencies = " << node.getValue<string>() << endl;
		}
		else if( node.getTag() == "AdditionalLibraryDirectories" ) {
			console() << "AdditionalLibraryDirectories = " << replaceVcxprojMacros( node.getValue<string>(), config ) << endl;
		}
		// skip
		else if( node.getTag() == "ResourceCompile" ) {
			return;
		}
			
		for( const auto &child : node.getChildren() ) {
			parseVcxproj( *child, config, matched );
		}
	}
	
	void getProjectConfigurationTree( const XmlTree &node, XmlTree* results, const ProjectConfiguration &config )
	{
		for( const auto &child : node.getChildren() ) {
			if( child->hasAttribute( "Condition" ) && matchCondition( child->getAttributeValue<string>( "Condition" ), config ) ) {
				//results->push_back( *child );
				//parseVcxprojXmlNode( *child );
			}
			else if( ! child->hasAttribute( "Condition" ) ) {
				XmlTree copy = *child;
				
				copy.getChildren().clear();
				getProjectConfigurationTree( *child, &copy, config );
				//parseVcxprojXmlNode( copy );
				//parseVcxprojXmlNode( *child );
				//results->push_back( copy );
			}
		}
	}

	/*rt::CompilerMsvc::BuildSettings generateBuildSettings( const ci::fs::path &vcxProj, const ProjectConfiguration &config )
	{
		rt::CompilerMsvc::BuildSettings buildSettings;

		return buildSettings;
	}*/
}

void LiveApp::setup()
{
	//fs::path projectPath;
	//fs::path appPath = getAppPath();
	//size_t depth = 0;
	//for( fs::path path = appPath; path.has_parent_path() || ( path == appPath ); path = path.parent_path(), ++depth ) {
	//	if( depth >= 5 || ! projectPath.empty() )
	//		break;

	//	for( fs::directory_iterator it = fs::directory_iterator( path ), end; it != end; ++it ) {
	//		if( it->path().extension() == ".vcxproj" ) {
	//			projectPath = it->path();
	//			break;
	//		}
	//	}
	//}
	//
	////	
	////CI_RT_CINDER_PATH=ci::fs::path( "../../../../.." )
	////CI_RT_PROJECT_ROOT=ci::fs::path( R"($(ProjectDir))" ) / ".."
	////CI_RT_PROJECT_DIR=ci::fs::path( R"($(ProjectDir))" )
	////CI_RT_PROJECT_PATH=ci::fs::path( R"($(ProjectPath))" )
	////CI_RT_PLATFORM_TARGET=R"($(PlatformTarget))"
	////CI_RT_CONFIGURATION=R"($(Configuration))"
	////CI_RT_PLATFORM_TOOLSET=R"($(PlatformToolset))"
	////CI_RT_INTERMEDIATE_DIR=ci::fs::path( R"($(IntDir))" )
	////CI_RT_OUTPUT_DIR=ci::fs::path( R"($(OutDir))" )

	//ProjectConfiguration projectConfig;
	//projectConfig.projectPath = projectPath;
	//projectConfig.projectDir = projectPath.parent_path();
	//console() << "$(Configuration) = " << projectConfig.configuration << endl;
	//console() << "$(Platform) = " << projectConfig.platform << endl;
	//console() << "$(ProjectPath) = " << projectConfig.projectPath << endl;
	//console() << "$(ProjectDir) = " << projectConfig.projectDir << endl;

	////string condition = "Debug|x64";
	//string condition = "'$(Configuration)|$(Platform)'=='Debug_Shared|x64'";
	//
	//
	////console() << condition << " " << boolalpha << matchCondition( condition, projectConfig ) << endl;

	//if( ! projectPath.empty() ) {
	//	
	//	parseVcxproj( XmlTree( loadFile( projectPath ) ), projectConfig );
	//	//console() << configXml << endl;
	//		
	//		/*if( child->hasAttribute( "Condition" ) ? child->getAttribute( "Condition" ).getValue<string>().find( condition ) != string::npos : true ) {
	//			for( const auto &cchild : child->getChildren() ) {
	//				console() << *cchild << endl;
	//			}
	//		}*/
	//		
	//	//}
	//}
}

void LiveApp::draw()
{
	gl::clear( ColorA::gray( 0.75f ) );
}

CINDER_RUNTIME_APP( LiveApp, RendererGl )