#include "common.h"
#include "PostTokenizer.h"
#include "PPTokenizer.h"
#include "PostTokenReceiver.h"
#include "PPDirective.h"
#include "BuildEnv.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>

using namespace std;
using namespace compiler;

int main()
{
  BuildEnv buildEnv;
	try
	{
    using namespace std::placeholders;
    ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

    PostTokenReceiver postTokenReceiver([](const PostToken& token) {
      if (token.getType() != PostTokenType::NewLine) {
        cout << token.toStr() << endl;
      }
    });
    PostTokenizer postTokenizer(postTokenReceiver);

    PPDirective ppDirective(bind(&PostTokenizer::put, 
                                 &postTokenizer,
                                 _1),
                            buildEnv);

		PPTokenizer ppTokenizer;
    ppTokenizer.sendTo(bind(&PPDirective::put,
                            &ppDirective,
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

