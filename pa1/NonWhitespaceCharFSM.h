#pragma once

#include "StateMachine.h"
#include "common.h"
#include <vector>

namespace compiler {

class NonWhiteSpaceCharFSM : public StateMachine
{
public:
  StateMachine* put(int x) override {
    if (x == '"' || x == '\'') {
      Throw("Unmatched `{}`", static_cast<char>(x));
    }
    send_(PPToken(PPTokenType::NonWhitespaceChar, {x}));
    return this;
  }
};

}
