#pragma once

#include "StateMachine.h"
#include <vector>

namespace compiler {

class RawStringLiteralFSM : public StateMachine
{
public:
  StateMachine* put(int x) override;
  StateMachine* put(const std::vector<int>& ch) override;
private:
  bool extend(int x);
  std::vector<int> ch_;
  bool rChar_ { false };
  std::vector<int> dChar_;
};

}
