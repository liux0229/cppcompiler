#include "StateMachine.h"
#include "common.h"

namespace compiler {

class NewLineFSM : public StateMachine
{
public:
  bool put(int x) override {
    if (x != '\n') {
      return false;
    } else {
      accepted_ = true;
      return true;
    }
  }
  PPToken get() const override { 
    if (!accepted_) {
      Throw("NewLineFSM not accepted yet");
    }

    return PPToken{PPTokenType::NewLine};
  }
private:
  bool accepted_ { false };
};

}
