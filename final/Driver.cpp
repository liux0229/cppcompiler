#include "Driver.h"
#include "parsers/SemanticParser.h"

#include <exception>

namespace compiler {

using namespace std;

void Driver::postTokenProcessor(const Token& token) {
  if (token.isSimple()) {
    if (static_cast<const TokenSimple&>(token).type == OP_RSHIFT) {
      tokens_.push_back(make_unique<TokenSimple>(">", OP_RSHIFT_1));
      tokens_.push_back(make_unique<TokenSimple>(">", OP_RSHIFT_2));
      return;
    }
  } else if (token.isNewLine()) {
    return;
  }
  // TODO: avoid this copying
  tokens_.push_back(token.copy());
}

UTranslationUnit Driver::process() {
  try {
    preprocessor_.process();
    return SemanticParser(tokens_, parserOption_).process();
  } catch (const exception& e) {
		cerr << "ERROR: " << e.what() << endl;
    return nullptr;
  }
}

}
