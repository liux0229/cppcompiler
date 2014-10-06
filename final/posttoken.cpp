#include "common.h"
#include "Tokenizer.h"
#include "PPTokenizer.h"
#include "PostTokenReceiver.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>

using namespace std;
using namespace compiler;

int main()
{
	try
	{
    ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

		ppToken::PPTokenizer ppTokenizer;
    TokenReceiver postTokenReceiver([](const Token& token) {
      if (token.getType() != TokenType::NewLine) {
        cout << token.toStr() << endl;
      }
    });
    Tokenizer postTokenizer(postTokenReceiver);
    ppTokenizer.sendTo(bind(&Tokenizer::put, 
                            &postTokenizer,
                            placeholders::_1));

		for (char c : input)
		{
			unsigned char code_unit = c;
			ppTokenizer.process(code_unit);
		}

		ppTokenizer.process(EndOfFile);
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}
