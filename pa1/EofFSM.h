#include "StateMachine.h"
#include "common.h"

namespace compiler {

class EofFSM : public StateMachine
{
public:
  bool put(int x) override {
    if (x != EndOfFile) {
      return false;
    } else {
      accepted_ = true;
      return true;
    }
  }
  PPToken get() const override { 
    if (!accepted_) {
      Throw("EofFSM not accepted yet");
    }

    return {PPTokenType::Eof};
  }
private:
  bool accepted_ { false };
};

}
