#include "PPDirective.h"
#include "PPDirectiveUtil.h"
#include "common.h"

namespace compiler {

using namespace std;

void PPDirective::handleDirective()
{
  if (directive_.empty()) {
    Throw("empty preprocessing directive");
  }
  auto& start = directive_[0];
  if (start.type != PPTokenType::Identifier) {
    Throw("preprocessing directive does not start with an identifier");
  }
  string dir = start.dataStrU8();
  if (dir == "define" || dir == "undef") {
    macroProcessor_.def(directive_);
  } else {
    Throw("bad preprocessing directive {}", dir);
  }
  directive_.clear();
}

void PPDirective::handleExpand()
{
  if (text_.empty()) {
    return;
  }
  vector<PPToken> expanded = macroProcessor_.expand(text_);
  text_.clear();

  for (auto& token : expanded) {
    send_(token);
  }
}

void PPDirective::put(const PPToken& token)
{
  bool isText = true;
  if (token.type == PPTokenType::Eof) {
    handleExpand();
    send_(token);
    return;
  } else if (token.type == PPTokenType::NewLine) {
    if (state_ == 1) {
      handleDirective();
      isText = false;
    }
    state_ = 0;
  } else {
    switch (state_) {
      case 0:
        if (isPound(token)) {
          handleExpand();
          state_ = 1;
          isText = false;
        } else if (token.type != PPTokenType::WhitespaceSequence) {
          state_ = 2;
        } 
        // else: white-space, state_ is still 0
        break;
      case 1:
        directive_.push_back(token);
        isText = false;
        break;
      default:
        CHECK(state_ == 2);
        // collect the token into text
        break;
    }
  }
  if (isText) {
    text_.push_back(token);
  }
}
}
