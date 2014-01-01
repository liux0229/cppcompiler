#include "common.h"
#include "PostTokenizer.h"
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

		PPTokenizer ppTokenizer;
    PostTokenReceiver postTokenReceiver([](const PostToken& token) {
      if (token.getType() != PostTokenType::NewLine) {
        cout << token.toStr() << endl;
      }
    });
    PostTokenizer postTokenizer(postTokenReceiver);
    ppTokenizer.sendTo(bind(&PostTokenizer::put, 
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
