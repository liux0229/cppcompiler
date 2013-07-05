#pragma once
#include "PreprocessingToken.h"
#include <string>

namespace compiler {

class PredefinedMacros
{
public:
  PredefinedMacros();
  PPToken get(const std::string& name);
};

}
