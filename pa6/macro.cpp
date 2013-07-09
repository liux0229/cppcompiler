#include "common.h"
#include "PostTokenizer.h"
#include "PPTokenizer.h"
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
    PostTokenReceiver postTokenReceiver([](const PostToken& token) {
      if (token.getType() != PostTokenType::NewLine) {
        cout << token.toStr() << endl;
      }
    });
    PostTokenizer postTokenizer(postTokenReceiver);

    SourceReader sourceReader(cin, "[stdin]");
    PPDirective ppDirective(bind(&PostTokenizer::put, 
                                 &postTokenizer,
                                 placeholders::_1),
                            buildEnv,
                            &sourceReader);

    PPTokenizer ppTokenizer;
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

