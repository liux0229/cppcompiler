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

int main()
{
	try
	{
    ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

    using namespace std::placeholders;
		PPTokenizer ppTokenizer;
    CtrlExprEval ctrlExprEval;
    PostTokenReceiver receiver(bind(&CtrlExprEval::put, &ctrlExprEval, _1));
    PostTokenizer postTokenizer(receiver);
    ppTokenizer.sendTo(bind(&PostTokenizer::put, 
                            &postTokenizer,
                            _1));

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

