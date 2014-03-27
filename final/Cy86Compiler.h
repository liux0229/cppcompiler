#pragma once 

#include "PostProcessingToken.h"

#include <vector>

namespace compiler {

class Cy86Compiler {
 public:
  std::vector<char> compile(std::vector<UToken>&& tokens);
 private:
};

}
