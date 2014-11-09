#include <fstream>

#include "parser/Parser.h"
#include "BuildEnv.h"
#include "common.h"
#include "Preprocessor.h"

using namespace std;
using namespace compiler;
using namespace compiler::parser;

namespace {

void parse(const string& source) {
  BuildEnv env;
  vector<UToken> tokens;

  // TODO: issue with this API: 
  // 1) Why do we take in token as const Token& ?
  // 2) Can we stream token into the parser without creating the vector first?
  Preprocessor processor(env, source, [&tokens](const Token& token) {
    if (token.isNewLine()) {
      return;
    }
    tokens.push_back(token.copy());
  });
  processor.process();

  Parser parser(move(tokens));
  parser.parse();
}

} // unnamed

int main(int argc, char** argv) {
  try
  {
    vector<string> args;
    for (int i = 1; i < argc; i++) {
      args.emplace_back(argv[i]);
    }
    // TODO: consider using GFLAG
    if (args.size() < 3 || args[0] != "-o") {
      throw logic_error("invalid usage");
    }

    string output = args[1];
    string input = args[2];
    parse(input);

    cerr << "hello" << endl;
    cerr << "|level 2" << endl;
    cerr << "||level 3" << endl;
  } catch (exception& e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  }
}