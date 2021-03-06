#include "Driver.h"
#include "parsers/SemanticParser.h"

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

bool Driver::process() {
  try {
    preprocessor_.process();
    frame_ = SemanticParser(tokens_, parserOption_).process();
    return true;
  } catch (const exception& e) {
		cerr << "ERROR: " << e.what() << endl;
    return false;
  }
}

const Namespace* Driver::getGlobalNamespace() const {
  return frame_->getGlobalNamespace();
}

}
