// (C) 2013 CPPGM Foundation www.cppgm.org.  All rights reserved.

#include "common.h"
#include "PostTokenizer.h"
#include "PPTokenizer.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>

using namespace std;
using namespace compiler;

int main()
{
	// TODO:
	// 1. apply your code from PA1 to produce `preprocessing-tokens`
	// 2. "post-tokenize" the `preprocessing-tokens` as described in PA2
	// 3. write them out in the PA2 output format specifed

	// You may optionally use the above starter code.
	//
	// In particular there is the DebugPostTokenOutputStream class which helps form the
	// correct output format:

	try
	{
    ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

		PPTokenizer ppTokenizer;
    PostTokenizer postTokenizer;
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
