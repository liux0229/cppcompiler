#include "Driver.h"
#include "SemanticParser.h"

#include <exception>

namespace compiler {

using namespace std;

void Driver::postTokenProcessor(const PostToken& token) {
  if (token.isSimple()) {
    if (static_cast<const PostTokenSimple&>(token).type == OP_RSHIFT) {
      tokens_.push_back(make_unique<PostTokenSimple>(">", OP_RSHIFT_1));
      tokens_.push_back(make_unique<PostTokenSimple>(">", OP_RSHIFT_2));
      return;
    }
  } else if (token.isNewLine()) {
    return;
  }
  // TODO: avoid this copying
  tokens_.push_back(token.copy());
}

void Driver::process() {
  try {
    preprocessor_.process();
    SemanticParser(tokens_, parserOption_).process();
  } catch (const exception& e) {
		cerr << "ERROR: " << e.what() << endl;
  }
}

const Namespace& Driver::getGlobalNs() const {
  return globalNs_;
}

}
