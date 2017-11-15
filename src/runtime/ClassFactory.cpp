#include "runtime/ClassFactory.h"
#include <fstream>

using namespace std;
using namespace ci;

namespace runtime {

void generateClassFactory( const ci::fs::path &outputPath, const std::string &className, const std::string &headerName )
{
	std::ofstream outputFile( outputPath );
	outputFile << "#include <new>" << endl;
	outputFile << "#include \"" << ( headerName.empty() ? className + ".h" : headerName ) << "\"" << endl << endl;
	outputFile << "extern \"C\" __declspec(dllexport) " << className << "* __cdecl rt_new_operator()" << endl;
	outputFile << "{" << endl;
	outputFile << "\treturn ::new " << className << "();" << endl;
	outputFile << "}" << endl;
	outputFile << endl;
	outputFile << "extern \"C\" __declspec(dllexport) void* __cdecl rt_placement_new_operator(  void* address )" << endl;
	outputFile << "{" << endl;
	outputFile << "\treturn static_cast<void*>( ::new (address) " << className << "() );" << endl;
	outputFile << "}" << endl;
	outputFile << endl;
}

}