#pragma once

#include "X86Instruction.h"

#include <memory>
#include <vector>

namespace compiler {

namespace Cy86 {

class Operand {
 public:
};

using UOperand = std::unique_ptr<Operand>;

class Register : public Operand {
 public:
  enum Type {
    SP, BP,
    X, Y, Z, T
  };
  Register(const std::string& name);
 private:
  void set(Type type, int size) {
    type_ = type;
    size_ = size;
  }

  Type type_;
  int size_; // # bits
};

class Cy86Instruction {
 public:
  Cy86Instruction(std::vector<UOperand>&& operands)
    : operands_(std::move(operands)) {
  }

  virtual std::vector<UX86Instruction> translate() = 0;
 private:
  const std::vector<UOperand> operands_;
};

using UCy86Instruction = std::unique_ptr<Cy86Instruction>;

class Cy86InstructionFactory {
 public:
  static UCy86Instruction get(const std::string& opcode, 
                              std::vector<UOperand>&& operands);
};

} // namespace Cy86

} // namespace compiler
