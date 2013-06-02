#pragma once

#include "StateMachine.h"
#include <vector>

namespace compiler {

class NonWhiteSpaceCharFSM : public StateMachine
{
public:
  StateMachine* put(int x) override {
    // for now, do not exclude " or '
    send_(PPToken(PPTokenType::NonWhitespaceChar, {x}));
    return this;
  }
};

}
