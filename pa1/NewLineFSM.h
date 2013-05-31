#include "StateMachine.h"
#include "common.h"

namespace compiler {

class NewLineFSM : public StateMachine
{
public:
  bool put(int x) override {
    if (x != '\n') {
      return false;
    }

    send_(PPToken(PPTokenType::NewLine));
    return true;
  }
};

}
