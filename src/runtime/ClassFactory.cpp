#include "runtime/ClassFactory.h"
#include <fstream>

using namespace std;
using namespace ci;

namespace runtime {

void generateClassFactory( const ci::fs::path &outputPath, const std::string &className, const std::string &headerName )
{
	std::ofstream outputFile( outputPath );		
	outputFile << "#include <memory>" << endl << endl;
	outputFile << "#include \"" << ( headerName.empty() ? className + ".h" : headerName ) << "\"" << endl << endl;
	outputFile << "extern \"C\" __declspec(dllexport) void __cdecl rt_make_shared( std::shared_ptr<" << className << ">* ptr )" << endl;
	outputFile << "{" << endl;
	outputFile << "\t*ptr = std::make_shared<" << className << ">();" << endl;
	outputFile << "}" << endl;
	outputFile << endl;
	outputFile << "extern \"C\" __declspec(dllexport) void __cdecl rt_make_unique( std::unique_ptr<" << className << ">* ptr )" << endl;
	outputFile << "{" << endl;
	outputFile << "\t*ptr = std::make_unique<" << className << ">();" << endl;
	outputFile << "}" << endl;
	outputFile << endl;
	outputFile << "extern \"C\" __declspec(dllexport) " << className << "* __cdecl rt_make_raw()" << endl;
	outputFile << "{" << endl;
	outputFile << "\treturn new " << className << "();" << endl;
	outputFile << "}" << endl;
	outputFile << endl;
}

}