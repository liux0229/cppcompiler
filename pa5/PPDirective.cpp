#include "PPDirective.h"
#include "PPDirectiveUtil.h"
#include "common.h"

namespace compiler {

using namespace std;

void PPDirective::handleDirective()
{
  size_t index = 0;
  while (index < directive_.size() && directive_[index].isWhite()) {
    ++index;
  }
  vector<PPToken> directive { directive_.begin() + index, directive_.end() };
  directive_.clear();

  if (directive.empty()) {
    Throw("empty preprocessing directive");
  }
  auto& start = directive[0];
  if (start.type != PPTokenType::Identifier) {
    Throw("preprocessing directive does not start with an identifier");
  }
  string dir = start.dataStrU8();
  if (dir == "define" || dir == "undef") {
    macroProcessor_.def(directive);
  } else {
    Throw("bad preprocessing directive {}", dir);
  }
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
