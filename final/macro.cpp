#include "common.h"
#include "Tokenizer.h"
#include "preprocessing_token/PPTokenizer.h"
#include "PostTokenReceiver.h"
#include "PPDirective.h"
#include "BuildEnv.h"
#include "SourceReader.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>

using namespace std;
using namespace compiler;

int main()
{
  BuildEnv buildEnv;
	try {
    TokenReceiver postTokenReceiver([](const Token& token) {
      if (token.getType() != TokenType::NewLine) {
        cout << token.toStr() << endl;
      }
    });
    Tokenizer postTokenizer(postTokenReceiver);

    SourceReader sourceReader(cin, "[stdin]");
    PPDirective ppDirective(bind(&Tokenizer::put, 
                                 &postTokenizer,
                                 placeholders::_1),
                            buildEnv,
                            &sourceReader);

    ppToken::PPTokenizer ppTokenizer;
    ppTokenizer.sendTo(bind(&PPDirective::put,
                       &ppDirective,
                       placeholders::_1));

    for (char c; sourceReader.get(c); )
    {
      ppTokenizer.process(static_cast<unsigned char>(c));
    }

    ppTokenizer.process(EndOfFile);
	} catch (exception& e) {
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}

