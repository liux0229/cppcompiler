#pragma once

#include "StateMachine.h"
#include <functional>

namespace compiler {

class IdentifierFSM : public StateMachine
{
public:
  StateMachine* put(int x) override;
  void setCanTransfer(std::function<bool ()> canTransfer) {
    canTransfer_ = canTransfer;
  }
private:
  StateMachine* tryTransfer(const std::vector<int>& ch);
  std::vector<int> ch_;
  std::function<bool ()> canTransfer_;
};

}
