#pragma once

#include "StateMachine.h"
#include "Utf8Utils.h"
#include <vector>

namespace compiler {

class WhiteSpaceFSM : public StateMachine
{
public:
  StateMachine* put(int x) override {
    if (Utf8Utils::isWhiteSpaceNoNewLine(x)) {
      has_ = true;
      return this;
    } else {
      if (has_) {
        send_(PPToken(PPTokenType::WhitespaceSequence));
        has_ = false;
      }
      return nullptr;
    }
  }
private:
  bool has_ { false };
};

}
