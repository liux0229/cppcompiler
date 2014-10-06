#include "StateMachine.h"
#include "common.h"

namespace compiler {

namespace ppToken {

class EofFSM : public StateMachine
{
public:
  StateMachine* put(int x) override {
    if (x != EndOfFile) {
      return nullptr;
    }

    send_(PPToken(PPTokenType::Eof));
    return this;
  }
};

} // ppToken

} // compiler
