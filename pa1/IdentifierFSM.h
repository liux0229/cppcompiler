#pragma once

#include "StateMachine.h"

namespace compiler {

class IdentifierFSM : public StateMachine
{
public:
  StateMachine* put(int x) override;
private:
  StateMachine* tryTransfer(const std::vector<int>& ch);
  std::vector<int> ch_;
};

}
