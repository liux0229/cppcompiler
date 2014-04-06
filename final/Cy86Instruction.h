#pragma once

#include "X86Instruction.h"
#include "ConstantValue.h"

#include <memory>
#include <vector>

namespace compiler {

namespace Cy86 {

class Operand {
 public:
  virtual ~Operand() { }
  virtual X86::UOperand toX86Operand(int size) const = 0;
  virtual bool isMemory() const { return false; }
  virtual bool isRegister() const { return false; }
  virtual bool isImmediate() const { return false; }
  virtual void output(std::ostream& out) const = 0;
  virtual bool writeable() const = 0;
};

using UOperand = std::unique_ptr<Operand>;
inline std::ostream& operator<<(std::ostream& out, const Operand& operand) {
  operand.output(out);
  return out;
}

class Register;
using URegister = std::unique_ptr<Register>;
class Register : public Operand {
 public:
  enum Type {
    SP, BP,
    X, Y, Z, T,
    // The following registers are only used internally for easier translation 
    // of cy86 instructions into x86 instructions
    ax, bx, cx, dx, di, si, r8, r9, r10, r11
  };

  Register(const std::string& name);
  Register(Type type, int size) {
    set(type, size);
  }

  static URegister Ax(int size) {
    return get(ax, size);
  }
  static URegister Ah() {
    auto r = get(ax, 8);
    r->hi8_ = true;
    return r;
  }
  static URegister Bx(int size) {
    return get(bx, size);
  }
  static URegister Cx(int size) {
    return get(cx, size);
  }
  static URegister Dx(int size) {
    return get(dx, size);
  }
  static URegister Rax() {
    return get(ax, 64);
  }
  static URegister Rbx() {
    return get(bx, 64);
  }
  static URegister Rcx() {
    return get(cx, 64);
  }
  static URegister Rdx() {
    return get(dx, 64);
  }
  static URegister Rdi() {
    return get(di, 64);
  }
  static URegister Rsi() {
    return get(si, 64);
  }
  static URegister R8() {
    return get(r8, 64);
  }
  static URegister R9() {
    return get(r9, 64);
  }
  static URegister R10() {
    return get(r10, 64);
  }
  static URegister R11() {
    return get(r11, 64);
  }

  X86::UOperand toX86Operand(int size) const override;
  void output(std::ostream& out) const override;
  bool isRegister() const override { return true; }
  bool writeable() const {
    return true;
  }
  bool hi8() const { return hi8_; }
  URegister toLower() const {
    CHECK(hi8_);
    return make_unique<Register>(type_, size_);
  }

  bool isR11() const { return type_ == r11; }

 private:
  static URegister get(Type type, int size) {
    return make_unique<Register>(type, size);
  }

  void set(Type type, int size) {
    type_ = type;
    size_ = size;
  }

  Type type_;
  int size_; // # bits
  bool hi8_ {false}; // Upper 8 bits of the 16 bit register
};

class Immediate : public Operand {
 public:
  Immediate(const std::string& label, SConstantValue literal) 
    : label_(label),
      literal_(literal) {
    CHECK(!label_.empty() || literal_);
  }
  X86::UOperand toX86Operand(int size) const override;
  bool isImmediate() const override { return true; }
  void output(std::ostream& out) const override;
  bool writeable() const {
    return false;
  }

  const std::string& label() const { return label_; }
  SConstantValue literal() const { return literal_; }

 private:
  const std::string label_;
  SConstantValue literal_;
};
using UImmediate = std::unique_ptr<Immediate>;

class Memory : public Operand {
 public:
  Memory(URegister reg, UImmediate imm)
    : reg_(std::move(reg)),
      imm_(std::move(imm)) {
  }
  bool isMemory() const override {
    return true;
  }
  bool writeable() const override {
    return true;
  }
  void output(std::ostream& out) const override;
  // TODO: This design is not regular since for memory we need special
  // transformations first
  X86::UOperand toX86Operand(int size) const override {
    MCHECK(false, "Memory::toX86Operand: Not implemented");
    return nullptr;
  }

  const Register* reg () { return &*reg_; }
  const Immediate* imm() { return &*imm_; }

 private:
  URegister reg_;
  UImmediate imm_;
};

class Cy86Instruction {
 public:
  Cy86Instruction(int size, std::vector<UOperand>&& operands)
    : size_(size),
      operands_(std::move(operands)) {
  }
  virtual ~Cy86Instruction() { }

  void addLabel(const std::string& label) {
    label_.push_back(label);
  }
  const std::vector<std::string>& getLabel() const {
    return label_;
  }
  virtual std::vector<X86::UX86Instruction> translate() = 0;

 protected:
  void checkOperandNumber(const std::string& name, size_t n) const;
  void checkWriteable(const std::string& name) const;

  int size() const { return size_; }
  std::vector<UOperand>& operands() {
    return operands_;
  }
  const std::vector<UOperand>& operands() const {
    return operands_;
  }
 private:
  int size_;
  std::vector<UOperand> operands_;
  std::vector<std::string> label_;
};

using UCy86Instruction = std::unique_ptr<Cy86Instruction>;

class Cy86InstructionFactory {
 public:
  static UCy86Instruction get(const std::string& opcode, 
                              std::vector<UOperand>&& operands);
};

} // namespace Cy86

} // namespace compiler
