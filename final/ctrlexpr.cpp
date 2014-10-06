#include "common.h"
#include "PostTokenizer.h"
#include "PPTokenizer.h"
#include "PostTokenReceiver.h"
#include "CtrlExprEval.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <string>

using namespace std;
using namespace compiler;

// mock implementation of IsDefinedIdentifier for PA3
// return true iff first code point is odd
bool PA3Mock_IsDefinedIdentifier(const string& identifier)
{
	if (identifier.empty())
		return false;
	else
		return identifier[0] % 2;
}


int main()
{
	try
	{
    ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

    using namespace std::placeholders;
		ppToken::PPTokenizer ppTokenizer;
    CtrlExprEval ctrlExprEval(true /* print result */,
                              PA3Mock_IsDefinedIdentifier);
    TokenReceiver receiver(bind(&CtrlExprEval::put, &ctrlExprEval, _1));
                        
    Tokenizer postTokenizer(receiver, true /* noStrCatForNewLine */);
    ppTokenizer.sendTo(bind(&Tokenizer::put, 
                            &postTokenizer,
                            _1));

		for (char c : input)
		{
			unsigned char code_unit = c;
			ppTokenizer.process(code_unit);
		}

		ppTokenizer.process(EndOfFile);
    cout << "eof" << endl;
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

