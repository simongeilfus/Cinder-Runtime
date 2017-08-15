#include "runtime/PrecompiledHeader.h"

using namespace std;
using namespace ci;

namespace runtime {

void generatePrecompiledHeader( const ci::fs::path &outputPath, const std::string &className, const std::string &headerName )
{
	//std::ifstream inputFile( cppPath );
	//std::stringstream pchHeaderStream;
	//std::stringstream pchSourceStream;
	//std::vector<string> pchIncludes;
	//	
	//// prepare pch files
	//pchHeaderStream << "#pragma once" << endl << endl;
	//pchSourceStream << "#include \"" << ( className + "PCH.h" ) << "\"" << endl;
	//		
	//// parse the source file
	//int skippedIncludes = 0;
	//for( string line; std::getline( inputFile, line ); ) {
	//	// if line is an include and is not including the main file move it to the pch header
	//	if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
	//		pchHeaderStream << line << endl;
	//		pchIncludes.push_back( line );
	//	}
	//	// #pragma hdrstop support
	//	else if( line.find( "#pragma") != string::npos && line.find( "hdrstop" ) != string::npos ) {
	//		break;
	//	}
	//}

	//// check if the pch header needs to be written for the first time or updated
	//bool writePchHeader = false;
	//bool pchHeaderExists = fs::exists( tempFolder / ( className + "PCH.h" ) );
	//if( pchHeaderExists ) {
	//	std::vector<string> pchHeaderFileIncludes;
	//	std::ifstream pchHeaderFile( tempFolder / ( className + "PCH.h" ) );
	//	for( string line; std::getline( pchHeaderFile, line ); ) {
	//		if( line.find( "#include" ) != string::npos && line.find( className + ".h" ) == string::npos ) {
	//			pchHeaderFileIncludes.push_back( line );
	//		}				
	//	}
	//	writePchHeader = ( pchIncludes != pchHeaderFileIncludes );
	//}
	//if( writePchHeader || ! pchHeaderExists ) {
	//	std::ofstream pchHeaderFile( tempFolder / ( className + "PCH.h" ) );
	//	pchHeaderFile << pchHeaderStream.str();
	//	createPch = true;
	//}

	//// check if the pch source file needs to be written for the first time
	//if( ! fs::exists( tempFolder / ( className + "PCH.cpp" ) ) ) {
	//	std::ofstream pchSourceFile( tempFolder / ( className + "PCH.cpp" ) );
	//	pchSourceFile << pchSourceStream.str();			
	//	createPch = true;
	//}
}

} // namespace runtime