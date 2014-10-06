#pragma once

#include "StateMachine.h"
#include <vector>

namespace compiler {

namespace ppToken {

class RawStringLiteralFSM : public StateMachine
{
public:
  StateMachine* put(int x) override;
  StateMachine* put(const std::vector<int>& ch) override;
  bool inside() const { return !ch_.empty(); }
private:
  bool extend(int x);
  std::vector<int> ch_;
  bool rChar_ { false };
  std::vector<int> dChar_;
};

} // ppToken

} // compiler
