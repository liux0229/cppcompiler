#pragma once
#include "PreprocessingToken.h"
#include "BuildEnv.h"
#include <string>
#include <map>

namespace compiler {

class PredefinedMacros
{
public:
  PredefinedMacros(BuildEnv buildEnv);
  PPToken get(const std::string& name) const;
private:
  std::map<std::string, PPToken> macros_; 
};

}
