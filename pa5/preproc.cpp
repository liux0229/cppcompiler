#include "common.h"
#include "Preprocessor.h"
#include "BuildEnv.h"
#include <utility>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>

using namespace std;
using namespace compiler;

// OPTIONAL: Also search `PA5StdIncPaths`
// on `--stdinc` command-line switch (not by default)
vector<string> PA5StdIncPaths =
{
    "/usr/include/c++/4.7/",
    "/usr/include/c++/4.7/x86_64-linux-gnu/",
    "/usr/include/c++/4.7/backward/",
    "/usr/lib/gcc/x86_64-linux-gnu/4.7/include/",
    "/usr/local/include/",
    "/usr/lib/gcc/x86_64-linux-gnu/4.7/include-fixed/",
    "/usr/include/x86_64-linux-gnu/",
    "/usr/include/"
};

void process(BuildEnv buildEnv, const string& source, ostream& out)
{
  Preprocessor processor(buildEnv, source, out);
  processor.process();
}

int main(int argc, char** argv)
{
  BuildEnv buildEnv;
	try
	{
		vector<string> args;
		for (int i = 1; i < argc; i++)
			args.emplace_back(argv[i]);

		if (args.size() < 3 || args[0] != "-o")
			throw logic_error("invalid usage");

		string outfile = args[1];
		size_t nsrcfiles = args.size() - 2;

		ofstream out(outfile);

		out << "preproc " << nsrcfiles << endl;
		for (size_t i = 0; i < nsrcfiles; i++)
		{
			string srcfile = args[i+2];
			out << "sof " << srcfile << endl;
			ifstream in(srcfile);
      process(buildEnv, srcfile, out);
			// out << "eof" << endl;
		}
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

