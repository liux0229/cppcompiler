#pragma once 

#include "PostProcessingToken.h"

#include <vector>
#include <tuple>

namespace compiler {

class Cy86Compiler {
 public:
   std::pair<std::vector<char>, size_t>
   compile(std::vector<UToken>&& tokens);
 private:
};

}
