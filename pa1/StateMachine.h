#pragma once

#include "PreprocessingToken.h"
#include <functional>
#include <vector>

namespace compiler {

class StateMachine;

class StateMachine
{
public:
  // return which state machine handled the character
  virtual StateMachine* put(int x) = 0;

  // handle a list of ch's
  virtual StateMachine* put(const std::vector<int>& ch) { return nullptr; }

  void sendTo(std::function<void (const PPToken&)> send) {
    send_ = send;
  }

  void setTransfer(std::vector<StateMachine*> transfer) {
    transfer_.swap(transfer);
  }
protected:
  std::function<void (const PPToken&)> send_;
  std::vector<StateMachine*> transfer_;
};

}
