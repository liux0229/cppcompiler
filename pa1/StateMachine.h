#pragma once

#include "PreprocessingToken.h"
#include <functional>

namespace compiler {

class StateMachine
{
public:
  virtual bool put(int x) = 0;
  void sendTo(std::function<void (const PPToken&)> send) {
    send_ = send;
  }
protected:
  std::function<void (const PPToken&)> send_;
};

}
