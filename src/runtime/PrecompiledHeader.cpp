#include "runtime/PrecompiledHeader.h"
#include "cinder/app/App.h"
#include <fstream>
#include <sstream>

using namespace std;
using namespace ci;

namespace runtime {

namespace {

std::vector<string> extractHeaderLines( const ci::fs::path &inputPath, const std::string &className )
{
	std::vector<string> includes;
	if( ci::fs::exists( inputPath ) ) {
		std::ifstream inputFile( inputPath );
		for( string line; std::getline( inputFile, line ); ) {
			// if line is an include and is not including the main file move it to the pch header
			if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
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

}

bool generatePrecompiledHeader( const ci::fs::path &inputCpp, const ci::fs::path &outputHeader, const ci::fs::path &outputCpp, bool force )
{
	auto className = inputCpp.stem().string();
	auto pchIncludes = extractHeaderLines( outputHeader, className );
	auto cppIncludes = extractHeaderLines( inputCpp, className );
	auto headerIncludes = extractHeaderLines( inputCpp.parent_path() / inputCpp.stem().replace_extension( ".h" ), className );
	cppIncludes.insert( cppIncludes.end(), headerIncludes.begin(), headerIncludes.end() );
	
	bool createPch = false;
	if( cppIncludes.size() ) {
		// check if the pch header needs to be written for the first time or updated
		if( force || pchIncludes != cppIncludes || ! ci::fs::exists( outputHeader ) ) {
			std::ofstream pchHeaderFile( outputHeader );
			pchHeaderFile << "#pragma once" << endl << endl;
			for( auto inc : cppIncludes ) {
				pchHeaderFile << inc << endl;
			}
			createPch = true;
		}
	
		// check if the pch source file needs to be written for the first time
		if( force || ! ci::fs::exists( outputCpp ) ) {
			std::ofstream pchSourceFile( outputCpp );
			pchSourceFile << "#include \"" << ( className + "Pch.h" ) << "\"" << endl;
			createPch = true;
		}
	}	

	return createPch;

		/*	
	// parse the source file
	int skippedIncludes = 0;
	for( string line; std::getline( inputFile, line ); ) {
		// if line is an include and is not including the main file move it to the pch header
		app::console() << line << endl;
		if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
			pchHeaderStream << line << endl;
			pchIncludes.push_back( line );
			app::console() << ">>>>>>>>>>>>>>>>>>" << line << endl;
		}
		// #pragma hdrstop support
		else if( line.find( "#pragma") != string::npos && line.find( "hdrstop" ) != string::npos ) {
			break;
		}
	}

	bool writePchHeader = false;
	app::console() << endl << "looking For " << inputCpp.parent_path() / inputCpp.stem().replace_extension( ".h" ) << endl;
	bool pchHeaderExists = fs::exists( inputCpp.parent_path() / inputCpp.stem().replace_extension( ".h" ) );
	if( pchHeaderExists ) {
		std::vector<string> pchHeaderFileIncludes;
		std::ifstream pchHeaderFile( inputCpp.parent_path() / inputCpp.stem().replace_extension( ".h" ) );
		for( string line; std::getline( pchHeaderFile, line ); ) {
		app::console() << line << endl;
			if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
				pchHeaderFileIncludes.push_back( line );
		app::console() << ">>>>>>>>>>>>>>>>>>" <<line << endl;
			}				
		}
		writePchHeader = ( pchIncludes != pchHeaderFileIncludes );
	}
	if( writePchHeader || ! pchHeaderExists ) {
		std::ofstream pchHeaderFile( outputHeader );
		pchHeaderFile << pchHeaderStream.str();
		//createPch = true;
	}

	if( ! fs::exists( outputCpp ) ) {
		std::ofstream pchSourceFile( outputCpp );
		pchSourceFile << pchSourceStream.str();			
		//createPch = true;
	}*/
}

} // namespace runtime