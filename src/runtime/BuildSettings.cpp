/*
 Copyright (c) 2017, Simon Geilfus
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "runtime/BuildSettings.h"
#include "runtime/ProjectConfiguration.h"
#include "cinder/Log.h"
#include "cinder/Xml.h"

using namespace std;
using namespace ci;

namespace runtime {

BuildSettings::BuildSettings()
: mVerbose( false ), mLinkAppObjs( true ), mCreatePch( false ), mUsePch( false )
{
}
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
		replaceAll( conditionLhs, "$(Configuration)", config.getConfiguration() );
		replaceAll( conditionLhs, "$(Platform)", config.getPlatform() );
		return ( conditionLhs == conditionRhs );
	};

	string replaceVcxprojMacros( const std::string &input, BuildSettings* settings, const ProjectConfiguration &config )
	{
		string output = input;

		// TODO: we may want these to be added to a map of Macros, which includes both user and system (similar to Visual Studio's 'Macros' pane)
		replaceAll( output, "$(Configuration)", config.getConfiguration() );
		replaceAll( output, "$(Platform)", config.getPlatform() );
		replaceAll( output, "$(PlatformTarget)", config.getPlatformTarget() );
		replaceAll( output, "$(PlatformToolset)", config.getPlatformToolset() );
		replaceAll( output, "$(ProjectDir)", config.getProjectDir().string() + "/" );

		// replace user macros
		for( const auto &macro : settings->getUserMacros() ) {
			replaceAll( output, macro.first, macro.second );
		}

		return output;
	}

	// Note: currently only supporting parsing UserMacros for property sheets, and each property sheet parsed overrides the previous one.
	// TODO: look into supporting the rest of prop sheet features
	void parsePropertySheet( BuildSettings* settings, const fs::path &fullPath )
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

	void parseVcxproj( BuildSettings* settings, const XmlTree &node, const ProjectConfiguration &config, bool matched = false )
	{
		if( ! matched && node.hasAttribute( "Condition" ) ) {
			matched = matchCondition( node.getAttributeValue<string>( "Condition" ), config );

			if( ! matched ) {
				return;
			}
		}

		if( node.getTag() == "OutDir" ) {
			auto outDir = fs::path( replaceVcxprojMacros( node.getValue<string>(), settings, config ) );
//			settings->outputPath( outDir.parent_path() );
		}
		else if( node.getTag() == "IntDir" ) {
			auto intDir = fs::path( replaceVcxprojMacros( node.getValue<string>(), settings, config ) );
			settings->intermediatePath( intDir.parent_path() );
		}
		else if( node.getTag() == "LinkIncremental" ) {
			//console() << "LinkIncremental = " << node.getValue<string>() << endl;
		}
		else if( node.getTag() == "AdditionalIncludeDirectories" ) {
			vector<string> includes = ci::split( replaceVcxprojMacros( node.getValue<string>(), settings, config ), ";" );
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
			vector<string> libraries = ci::split( replaceVcxprojMacros( librariesString, settings, config ), ";" );
			for( const auto &lib : libraries ) {
				if( ! lib.empty() ) {
					settings->library( lib );
				}
			}
		}
		else if( node.getTag() == "AdditionalLibraryDirectories" ) {
			vector<string> libraryDirectories = ci::split( replaceVcxprojMacros( node.getValue<string>(), settings, config ), ";" );
			for( auto dir : libraryDirectories ) {
				if( ! dir.empty() ) {
					settings->libraryPath( fs::path( dir ) );
				}
			}
		}
		else if( node.getTag() == "ImportGroup" ) {
			// parse user property sheets
			if( node.getAttributeValue<string>( "Label" ) == "PropertySheets" ) {
				for( const auto &child : node.getChildren() ) {
					if( child->hasAttribute( "Label" ) ) {
						// skip base template prop sheet
						if( child->getAttributeValue<string>( "Label" ) == "LocalAppDataPlatform")
							continue;
					}

					string fileName = child->getAttributeValue<string>( "Project" );
					fs::path propSheetFullPath = config.getProjectDir() / fileName;

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

#if defined( CINDER_MSW )
BuildSettings& BuildSettings::vcxproj( const ci::fs::path &path )
{
	const auto &projConfig = ProjectConfiguration::instance();
	parseVcxproj( this, XmlTree( loadFile( projConfig.getProjectPath() ) ), projConfig );

	return configuration( projConfig.getConfiguration() ).platform( projConfig.getPlatform() ).platformToolset( projConfig.getPlatformToolset() )
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
	// app src folder
	.include( "../src" );
}

//BuildSettings& BuildSettings::vcxprojCpp( const ci::fs::path &path )
//{
//
//}
//BuildSettings& BuildSettings::vcxprojLinker( const ci::fs::path &path )
//{
//
//}
#endif

std::string BuildSettings::printToString() const
{
	stringstream str;

	str << "link app objs: " << mLinkAppObjs << ", create pch: " << mCreatePch << ", use pch: " << mUsePch << "\n";
	str << "precompiled header: " << mPrecompiledHeader << "\n";
	str << "output path: " << mOutputPath << "\n";
	str << "intermediate path: " << mIntermediatePath << "\n";
	str << "pdb path: " << mPdbPath << "\n";
	str << "module name: " << mModuleName << "\n";
	str << "type name: " << mTypeName << "\n";
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

BuildSettings& BuildSettings::include( const ci::fs::path &path )
{
	mIncludes.push_back( path );
	return *this;
}
BuildSettings& BuildSettings::libraryPath( const ci::fs::path &path )
{
	mLibraryPaths.push_back( path );
	return *this;
}
BuildSettings& BuildSettings::library( const std::string &library )
{
	mLibraries.push_back( library );
	return *this;
}
BuildSettings& BuildSettings::define( const std::string &definition )
{
	mPpDefinitions.push_back( definition );
	return *this;
}

BuildSettings& BuildSettings::usePrecompiledHeader( bool use /*const ci::fs::path &path*/ )
{
	mUsePch = use;
	//mPrecompiledHeader = path;
	return *this;
}
		
BuildSettings& BuildSettings::createPrecompiledHeader( bool create /*const ci::fs::path &path*/ )
{
	mCreatePch = create;
	//mPrecompiledHeader = path;
	return *this;
}

BuildSettings& BuildSettings::objectFile( const ci::fs::path &path )
{
	mObjectFilePath = path;
	return *this;
}
BuildSettings& BuildSettings::programDatabase( const ci::fs::path &path )
{
	mPdbPath = path;
	return *this;
}

BuildSettings& BuildSettings::compilerOption( const std::string &option )
{
	mCompilerOptions.push_back( option );
	return *this;
}
BuildSettings& BuildSettings::linkerOption( const std::string &option )
{
	mLinkerOptions.push_back( option );
	return *this;
}
BuildSettings& BuildSettings::userMacro( const std::string &name, const std::string &value )
{
	mUserMacros[name] = value;
	return *this;
}

BuildSettings& BuildSettings::verbose( bool enabled )
{
	mVerbose = enabled;
	return *this;
}
BuildSettings& BuildSettings::outputPath( const ci::fs::path &path )
{
	mOutputPath = path;
	return *this;
}
BuildSettings& BuildSettings::intermediatePath( const ci::fs::path &path )
{
	mIntermediatePath = path;
	return *this;
}
BuildSettings& BuildSettings::configuration( const std::string &option )
{
	mConfiguration = option;
	return *this;
}
BuildSettings& BuildSettings::platform( const std::string &option )
{
	mPlatform = option;
	return *this;
}
BuildSettings& BuildSettings::platformToolset( const std::string &option )
{
	mPlatformToolset = option;
	return *this;
}
BuildSettings& BuildSettings::moduleName( const std::string &name )
{
	mModuleName = name;
	return *this;
}
BuildSettings& BuildSettings::typeName( const std::string &typeName )
{ 
	mTypeName = typeName;
	return *this;
}

BuildSettings& BuildSettings::forceInclude( const std::string &filename )
{
	mForcedIncludes.push_back( filename );
	return *this;
}

BuildSettings& BuildSettings::additionalSource( const ci::fs::path &cppFile )
{
	mAdditionalSources.push_back( cppFile );
	return *this;
}
BuildSettings& BuildSettings::additionalSources( const std::vector<ci::fs::path> &cppFiles )
{
	mAdditionalSources.insert( mAdditionalSources.begin(), cppFiles.begin(), cppFiles.end() );
	return *this;
}

BuildSettings& BuildSettings::linkObj( const ci::fs::path &path )
{
	mObjPaths.push_back( path );
	return *this;
}
BuildSettings& BuildSettings::linkAppObjs( bool link )
{
	mLinkAppObjs = link;
	return *this;
}
	
BuildSettings& BuildSettings::preBuildStep( const BuildStepRef &customStep ) 
{
	mPreBuildSteps.push_back( customStep );
	return *this;
}

BuildSettings& BuildSettings::postBuildStep( const BuildStepRef &customStep ) 
{
	mPostBuildSteps.push_back( customStep );
	return *this;
}

} // namespace runtime