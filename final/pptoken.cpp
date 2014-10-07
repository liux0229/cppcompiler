#include "preprocessing_token/PPTokenizer.h"
#include "common.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace compiler;
using namespace compiler::ppToken;

void printToken(const PPToken& token) {
  if (token.type == PPTokenType::Eof) {
    cout << token.typeName() << endl;
  } else {
    string encoded = Utf8Encoder::encode(token.data);
    cout << format("{} {} {}", token.typeName(), encoded.size(), encoded) 
      << endl;
  }
}

int main()
{
	try
	{
    ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

		PPTokenizer tokenizer;
    tokenizer.sendTo(printToken);

		for (char c : input)
		{
			unsigned char code_unit = c;
			tokenizer.process(code_unit);
		}

		tokenizer.process(EndOfFile);
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}
