#pragma once

#include "PreprocessingToken.h"

namespace compiler {

class StateMachine
{
public:
  virtual bool put(int x) = 0;
  virtual PPToken get() const = 0;
};

}
