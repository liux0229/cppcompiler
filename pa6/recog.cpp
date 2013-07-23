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

bool PA6_IsClassName(const string& identifier)
{
	return identifier.find('C') != string::npos;
}

bool PA6_IsTemplateName(const string& identifier)
{
	return identifier.find('T') != string::npos;
}

bool PA6_IsTypedefName(const string& identifier)
{
	return identifier.find('Y') != string::npos;
}

bool PA6_IsEnumName(const string& identifier)
{
	return identifier.find('E') != string::npos;
}

bool PA6_IsNamespaceName(const string& identifier)
{
	return identifier.find('N') != string::npos;
}

bool DoRecog(BuildEnv env, const string& source)
{
  try {
    vector<UToken> tokens;
    Preprocessor processor(env, source, [&tokens](const PostToken& token) {
      if (token.isSimple()) {
        if (static_cast<const PostTokenSimple&>(token).type == OP_RSHIFT) {
          tokens.push_back(make_unique<PostTokenSimple>(">", OP_RSHIFT_1));
          tokens.push_back(make_unique<PostTokenSimple>(">", OP_RSHIFT_2));
          return;
        }
      }
      tokens.push_back(token.copy());
    });
    processor.process();

    Parser(tokens).process();
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

		if (args.size() < 3 || args[0] != "-o")
			throw logic_error("invalid usage");

		string outfile = args[1];
		size_t nsrcfiles = args.size() - 2;

		ofstream out(outfile);

		out << "recog " << nsrcfiles << endl;

		for (size_t i = 0; i < nsrcfiles; i++)
		{
			string srcFile = args[i+2];
      bool success = DoRecog(env, srcFile);
      out << format("{} {}\n", srcFile, (success ? "OK" : "BAD"));
		}
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

