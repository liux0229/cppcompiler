#include "BuildEnv.h"
#include "common.h"
#include "Driver.h"
#include "Linker.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iostream>

using namespace std;
using namespace compiler;

void newHandler() {
  CHECK(false);
}

int main(int argc, char** argv)
{
  set_new_handler(newHandler);

  BuildEnv env;
	try
	{
		vector<string> args;

		for (int i = 1; i < argc; i++) {
			args.emplace_back(argv[i]);
    }

    ParserOption option;
    if (hasCommandlineSwitch(args, "--trace")) {
      option.isTrace = true;
    }

    bool printDeclarations = false;
    if (hasCommandlineSwitch(args, "--print")) {
      printDeclarations = true;
    }

		if (args.size() < 3 || args[0] != "-o") {
			throw logic_error("invalid usage");
    }

		string outfile = args[1];
		size_t nsrcfiles = args.size() - 2;

		ofstream out(outfile);

    if (printDeclarations) {
      cout << nsrcfiles << " translation units" << endl;
    }

    Linker linker;
		for (size_t i = 0; i < nsrcfiles; i++)
		{
			string srcFile = args[i + 2];
      if (printDeclarations) {
        cout << "start translation unit " << srcFile << endl;
      }
      Driver driver(env, srcFile, option);
      auto tu = driver.process();
      if (!tu) {
        return EXIT_FAILURE;
      }
      if (printDeclarations) {
        cout << *tu->getGlobalNamespace();
        cout << "end translation unit" << endl;
      }
      linker.addTranslationUnit(move(tu));
		}

    auto image = linker.process();
    for (auto c : image) {
      out << c;
    }
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

