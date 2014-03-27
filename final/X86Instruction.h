#pragma once

#include <memory>
#include <vector>

namespace compiler {

class X86Instruction {
 public:
  virtual std::vector<char> assemble() = 0;
};

using UX86Instruction = std::unique_ptr<X86Instruction>;

}
