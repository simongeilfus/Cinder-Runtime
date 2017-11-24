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

#include "runtime/BuildStep.h"
#include "runtime/BuildSettings.h"
#include <fstream>

#include "cinder/app/App.h"
#include "cinder/Utilities.h"

using namespace std;
using namespace ci;

namespace runtime {

BuildStep::~BuildStep()
{
}

CodeGeneration::Options& CodeGeneration::Options::newOperator( const std::string &className )
{
	mNewOperators.push_back( className );
	return *this;
}
CodeGeneration::Options& CodeGeneration::Options::placementNewOperator( const std::string &className )
{
	mPlacementNewOperators.push_back( className );
	return *this;
}
CodeGeneration::Options& CodeGeneration::Options::include( const std::string &filename )
{
	mIncludes.push_back( filename );
	return *this;
}

CodeGeneration::CodeGeneration( const Options &options )
	: mOptions( options )
{
}

void CodeGeneration::execute( BuildSettings* settings ) const
{
	fs::path outputPath = settings->getIntermediatePath() / "runtime" / settings->getModuleName() / ( settings->getModuleName() + "Factory.cpp" );
	bool generate = true;
	if( fs::exists( outputPath ) ) {
		// if the file already exists compare the number of lines
		// TODO: Better file comparison
		ifstream input( outputPath ); 
		size_t currentLength = 0;
		for( string line; std::getline( input, line); ++currentLength );
		size_t newLength = 2 + mOptions.mIncludes.size() + ( mOptions.mNewOperators.size() ? 6 : 0 ) + mOptions.mNewOperators.size() * 3 + ( mOptions.mPlacementNewOperators.size() ? 6 : 0 ) + mOptions.mPlacementNewOperators.size() * 3;
		generate = newLength != currentLength;
	}
		
	if( generate ) {
		// update the compiler build settings
		settings->additionalSource( outputPath );

		// generate the source
		std::ofstream outputFile( outputPath );
		outputFile << "#include <new>" << endl;
		for( const auto &inc : mOptions.mIncludes ) {
			outputFile << "#include \"" << inc << "\"" << endl;
		}
		outputFile << endl;
	
		if( mOptions.mNewOperators.size() ) {
			outputFile << "extern \"C\" __declspec(dllexport) void* __cdecl rt_new_operator( const std::string &className )" << endl;
			outputFile << "{" << endl;
			outputFile << "\tvoid* ptr;" << endl;
			for( size_t i = 0; i < mOptions.mNewOperators.size(); ++i ) {
				outputFile << "\t" << ( i > 0 ? "else if" : "if" ) << "( className == \"" << mOptions.mNewOperators[i] << "\" ) {" << endl;
				outputFile << "\t\tptr = static_cast<void*>( ::new " << mOptions.mNewOperators[i] << "() );" << endl;
				outputFile << "\t}" << endl;
			}
			outputFile << "\treturn ptr;" << endl;
			outputFile << "}" << endl;
			outputFile << endl;
		}
	
		if( mOptions.mPlacementNewOperators.size() ) {
			outputFile << "extern \"C\" __declspec(dllexport) void* __cdecl rt_placement_new_operator( const std::string &className, void* address )" << endl;
			outputFile << "{" << endl;
			outputFile << "\tvoid* ptr;" << endl;
			for( size_t i = 0; i < mOptions.mPlacementNewOperators.size(); ++i ) {
				outputFile << "\t" << ( i > 0 ? "else if" : "if" ) << "( className == \"" << mOptions.mPlacementNewOperators[i] << "\" ) {" << endl;
				outputFile << "\t\tptr = static_cast<void*>( ::new (address) " << mOptions.mPlacementNewOperators[i] << "() );" << endl;
				outputFile << "\t}" << endl;
			}
			outputFile << "\treturn ptr;" << endl;
			outputFile << "}" << endl;
			outputFile << endl;
		}
	}
	else {
		// update the linker build settings
		settings->linkObj( outputPath.parent_path() / "build" / ( settings->getModuleName() + "Factory.obj" ) );
	}
}

namespace {
	std::vector<string> extractHeaderLines( const ci::fs::path &inputPath )
	{
		std::vector<string> includes;
		if( ci::fs::exists( inputPath ) ) {
			std::ifstream inputFile( inputPath );
			for( string line; std::getline( inputFile, line ); ) {
				// if line is an include
				if( line.find( "#include" ) != string::npos ) {
					includes.push_back( line );
				}
				// #pragma hdrstop support
				else if( line.find( "#pragma") != string::npos && line.find( "hdrstop" ) != string::npos ) {
					break;
				}
			}
		}
		return includes;
	}
} // anonymous namespace

PrecompiledHeader::Options& PrecompiledHeader::Options::parseSource( const ci::fs::path &path )
{
	for( const auto &filename : extractHeaderLines( path ) ) {
		mIncludes.push_back( filename );
	}
	return *this;
}
	
PrecompiledHeader::Options& PrecompiledHeader::Options::include( const std::string &filename, bool angleBrackets )
{
	mIncludes.push_back( string( "#include " ) + ( angleBrackets ? "<" : "\"" ) + filename + ( angleBrackets ? ">" : "\"" ) );
	return *this;
}

PrecompiledHeader::Options& PrecompiledHeader::Options::ignore( const std::string &filename, bool angleBrackets )
{
	mIgnoredIncludes.push_back( string( "#include " ) + ( angleBrackets ? "<" : "\"" ) + filename + ( angleBrackets ? ">" : "\"" ) );
	return *this;
}

PrecompiledHeader::PrecompiledHeader( const Options &options )
	: mOptions( options )
{
}

void PrecompiledHeader::execute( BuildSettings* settings ) const
{
	auto outputHeader = settings->getIntermediatePath() / "runtime" / settings->getModuleName() / ( settings->getModuleName() + "Pch.h" ); 
	auto outputCpp = settings->getIntermediatePath() / "runtime" / settings->getModuleName() / ( settings->getModuleName() + "Pch.cpp" ); 
	auto outputPch = settings->getIntermediatePath() / "runtime" / settings->getModuleName() / "build" / ( settings->getModuleName() + ".pch" ); 

	// filter out ignored includes
	mOptions.mIncludes.erase( remove_if( mOptions.mIncludes.begin(), mOptions.mIncludes.end(), [this](const std::string &filename) -> bool {
		 return std::find( mOptions.mIgnoredIncludes.begin(), mOptions.mIgnoredIncludes.end(), filename ) != mOptions.mIgnoredIncludes.end();
	} ), mOptions.mIncludes.end() );

	// don't generate anything if the include list is empty
	bool createPch = false;
	if( mOptions.mIncludes.size() ) {
		
		// check if the pch header needs to be written for the first time or updated
		auto pchIncludes = extractHeaderLines( outputHeader );
		if( pchIncludes != mOptions.mIncludes || ! ci::fs::exists( outputHeader ) ) {
			std::ofstream pchHeaderFile( outputHeader );
			pchHeaderFile << "#pragma once" << endl << endl;
			for( auto inc : mOptions.mIncludes ) {
				pchHeaderFile << inc << endl;
			}
			createPch = true;
		}
	
		// check if the pch source file needs to be written for the first time
		if( ! ci::fs::exists( outputCpp ) ) {
			std::ofstream pchSourceFile( outputCpp );
			pchSourceFile << "#include \"" << ( settings->getModuleName() + "Pch.h" ) << "\"" << endl;
			createPch = true;
		}
	}
	
	// update build settings accordingly
	if( createPch || ( fs::exists( outputCpp ) && ! fs::exists( outputPch ) ) ) {
		settings->createPrecompiledHeader( true );
		settings->usePrecompiledHeader( true );
		settings->forceInclude( settings->getModuleName() + "Pch.h" );
		settings->linkObj( settings->getIntermediatePath() / "runtime" / settings->getModuleName() / "build" / ( settings->getModuleName() + "Pch.obj" ) );
		app::console() << "Create and Use!!!" << endl;
	}
	else if( fs::exists( outputPch ) ) {
		settings->usePrecompiledHeader( true );
		settings->forceInclude( settings->getModuleName() + "Pch.h" );
		settings->linkObj( settings->getIntermediatePath() / "runtime" / settings->getModuleName() / "build" / ( settings->getModuleName() + "Pch.obj" ) );
		app::console() << "Use!!!" << endl;
	}
}

ModuleDefinition::Options& ModuleDefinition::Options::exportSymbol( const std::string &symbol )
{
	return *this;
}
ModuleDefinition::Options& ModuleDefinition::Options::exportVftable( const std::string &className )
{
	return *this;
}

// Examples: turns 'MyClass' into '??_7MyClass@@6B@', or 'a::b::MyClass' into '??_7MyClass@b@a@@6B@'
// See docs in generateLinkerCommand()
// See MS Doc "Decorated Names": https://msdn.microsoft.com/en-us/library/56h2zst2.aspx?f=255&MSPPError=-2147217396#Format
std::string ModuleDefinition::getVftableSymbol( const std::string &typeName )
{
	auto parts = ci::split( typeName, "::" );
	string decoratedName;
	for( auto rIt = parts.rbegin(); rIt != parts.rend(); ++rIt ) {
		if( ! rIt->empty() ) // handle leading "::" case, which results in any empty part
			decoratedName += *rIt + "@";
	}

	return "??_7" + decoratedName + "@6B@";
}
	
ModuleDefinition::ModuleDefinition( const Options &options )
{
}

void ModuleDefinition::execute( BuildSettings* settings ) const
{
}

} // namespace runtime