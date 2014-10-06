#include "StateMachine.h"
#include "common.h"

namespace compiler {

namespace ppToken {

class NewLineFSM : public StateMachine
{
public:
  StateMachine* put(int x) override {
    if (x != '\n') {
      return nullptr;
    }

    send_(PPToken(PPTokenType::NewLine));
    return this;
  }
};

} // ppToken

} // compiler
