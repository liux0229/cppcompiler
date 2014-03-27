#pragma once

#include "PostProcessingToken.h"
#include "Cy86Instruction.h"

#include <vector>

namespace compiler {

class Cy86Parser {
 public:
   std::vector<Cy86::UCy86Instruction> parse(std::vector<UToken>&& tokens);
};

}
