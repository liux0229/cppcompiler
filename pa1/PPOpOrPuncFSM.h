#pragma once

namespace compiler {

class PPOpOrPunc : public StateMachine
{
public:
  bool put(int x) override {
    // TODO: match prefix
    return true;
  }
};

}
