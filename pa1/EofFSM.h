#include "StateMachine.h"
#include "common.h"

namespace compiler {

class EofFSM : public StateMachine
{
public:
  bool put(int x) override {
    if (x != EndOfFile) {
      return false;
    }
    
    send_(PPToken(PPTokenType::Eof));
    return true;
  }
};

}
