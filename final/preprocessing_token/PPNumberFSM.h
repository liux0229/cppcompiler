#pragma once

#include "StateMachine.h"

namespace compiler {

namespace ppToken {

class PPNumberFSM : public StateMachine
{
public:
  StateMachine* put(int x) override;
private:
  bool canExtend(int c) const;
  std::vector<int> ch_;
};

} // ppToken

} // compiler
