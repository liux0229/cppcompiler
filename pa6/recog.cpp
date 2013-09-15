#include "BuildEnv.h"
#include "common.h"
#include "Preprocessor.h"
#include "Parser.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iostream>

using namespace std;
using namespace compiler;

bool DoRecog(BuildEnv env, const string& source, bool isTrace)
{
  try {
    vector<UToken> tokens;
    Preprocessor processor(env, source, [&tokens](const PostToken& token) {
      // TODO: consider moving this transformation stage to Parser
      if (token.isSimple()) {
        if (static_cast<const PostTokenSimple&>(token).type == OP_RSHIFT) {
          tokens.push_back(make_unique<PostTokenSimple>(">", OP_RSHIFT_1));
          tokens.push_back(make_unique<PostTokenSimple>(">", OP_RSHIFT_2));
          return;
        }
      } else if (token.isNewLine()) {
        return;
      }
      tokens.push_back(token.copy());
    });
    processor.process();

    Parser(tokens, isTrace).process();
    return true;
  } catch (const exception& e) {
		cerr << "ERROR: " << e.what() << endl;
    return false;
  }
};

int main(int argc, char** argv)
{
  BuildEnv env;
	try
	{
		vector<string> args;

		for (int i = 1; i < argc; i++)
			args.emplace_back(argv[i]);

    auto itTrace = find(args.begin(), args.end(), "--trace");
    bool isTrace = false;
    if (itTrace != args.end()) {
      isTrace = true;
      args.erase(itTrace);
    }

		if (args.size() < 3 || args[0] != "-o")
			throw logic_error("invalid usage");

		string outfile = args[1];
		size_t nsrcfiles = args.size() - 2;

		ofstream out(outfile);

		out << "recog " << nsrcfiles << endl;

		for (size_t i = 0; i < nsrcfiles; i++)
		{
			string srcFile = args[i+2];
      bool success = DoRecog(env, srcFile, isTrace);
      out << format("{} {}\n", srcFile, (success ? "OK" : "BAD"));
		}
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

