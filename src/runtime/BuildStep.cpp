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

PrecompiledHeader::Options& PrecompiledHeader::Options::parseSource( const ci::fs::path &path )
{
	//mIncludes.push_back( filename );
	return *this;
}
	
PrecompiledHeader::Options& PrecompiledHeader::Options::include( const std::string &filename )
{
	mIncludes.push_back( filename );
	return *this;
}

PrecompiledHeader::PrecompiledHeader( const Options &options )
	: mOptions( options )
{
}

void PrecompiledHeader::execute( BuildSettings* settings ) const
{

}

} // namespace runtime