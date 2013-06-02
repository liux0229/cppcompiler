#pragma once

#include "StateMachine.h"
#include <vector>

namespace compiler {

class CharacterLiteralFSM : public StateMachine
{
public:
  StateMachine* put(int x) override;
  StateMachine* put(const std::vector<int>& ch) override;
private:
  bool extend(int x);
  std::vector<int> ch_;
  int octal_ { 0 };
  bool hex_ { false };
};

}
