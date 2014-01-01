#include "BuildEnv.h"
#include "common.h"
#include "Driver.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iostream>

using namespace std;
using namespace compiler;

int main(int argc, char** argv)
{
  BuildEnv env;
	try
	{
		vector<string> args;

		for (int i = 1; i < argc; i++) {
			args.emplace_back(argv[i]);
    }

    ParserOption option;
    if (ParserOption::hasSwitch(args, "--trace")) {
      option.isTrace = true;
    }

		if (args.size() < 3 || args[0] != "-o") {
			throw logic_error("invalid usage");
    }

		string outfile = args[1];
		size_t nsrcfiles = args.size() - 2;

		ofstream out(outfile);

    out << nsrcfiles << " translation units" << endl;
		for (size_t i = 0; i < nsrcfiles; i++)
		{
			string srcFile = args[i + 2];
      out << "start translation unit " << srcFile << endl;
      Driver driver(env, srcFile, option);
      auto tu = driver.process();
      if (!tu) {
        return EXIT_FAILURE;
      }
      out << *tu->getGlobalNamespace();
      out << "end translation unit" << endl;
		}
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

