#pragma once

#include "common.h"
#include "ConstantValue.h"

#include <memory>
#include <vector>

namespace compiler {

namespace X86 {

struct MachineInstruction;
class Operand {
 public:
  virtual ~Operand() { }
  virtual int size() const = 0;
  virtual bool isRegister() const { return false; }
  virtual bool isImmediate() const { return false; }
  virtual bool isMemory() const { return false; }
};
using UOperand = std::unique_ptr<Operand>;

class Register : public Operand {
 public:
  enum Type {
    // note: the order of this list is specially constructed so that
    // the integeral value of the enumerators can directly be used 
    // in instruction encoding
    AX, CX, DX, BX,
    SP, BP, SI, DI,
    R8, R9, R10, R11,
    R12, R13, R14, R15
  };
  Register(Type type, int size)
    : type_(type),
      size_(size) {
  }

  bool isRegister() const override { return true; }

  int size() const override { return size_; }

  Type getType() const {
    return type_;
  }
 private:
  Type type_;
  int size_;
};
using URegister = std::unique_ptr<Register>;

#define GEN_REG(name, type, size) \
inline URegister name() { \
  return make_unique<Register>(Register::type, size);\
}

GEN_REG(Rax, AX, 64)
GEN_REG(Rdi, DI, 64)
GEN_REG(Rsi, SI, 64)
GEN_REG(R10, R10, 64)
GEN_REG(Rdx, DX, 64)
GEN_REG(R8, R8, 64)
GEN_REG(R9, R9, 64)

#undef GEN_REG

class Immediate : public Operand {
 public:
  Immediate(SConstantValue literal) 
    : literal_(literal) {
  }

  bool isImmediate() const override { return true; }

  int size() const override {
    return literal_->type->getTypeSize() * 8;
  }

  SConstantValue getLiteral() const {
    return literal_;
  }

 private:
  SConstantValue literal_;
};

// Assume always use [RDI]; will enhance in the future if necessary
class Memory : public Operand {
 public:
  Memory(int size) : size_(size) { }
  int size() const override { return size_; }
  bool isMemory() const override { return true; }
 private:
  int size_;
};

struct MachineInstruction {
  // TODO: consider making many of the following fields private
  struct Rex {
    Rex& operator|=(Rex other) {
      W |= other.W;
      R |= other.R;
      X |= other.X;
      B |= other.B;
      return *this;
    }
    unsigned char toByte() const;

    bool W;
    bool R;
    bool X;
    bool B;
  };
  static const Rex RexW;
  static const Rex RexR;
  static const Rex RexX;
  static const Rex RexB;

  struct ModRm {
    unsigned char toByte() const;

    int mod {0};
    int reg {0};
    int rm {0};
  };

  void addRex(Rex r);
  void setModRmRegister(Register::Type reg);
  void setModRmMemory();
  void setReg(Register::Type reg);
  void setImmediate(SConstantValue imm);
  std::vector<unsigned char> toBytes() const;

  void initModRm();

  std::vector<Rex> rex;
  std::vector<unsigned char> opcode;
  std::vector<ModRm> modRm;
  std::vector<unsigned char> immediate;
};

inline MachineInstruction::Rex operator|(MachineInstruction::Rex a,
                                         MachineInstruction::Rex b) {
  a |= b;
  return a;
}

// Plan:
// 1. implement syscall (basic version). so we can test read and write
// 2. Expand that into memory version
// 2.5 full move support
// 3. Add arithmetic support
// 4. Add label support
// 5. Add floating support

class X86Instruction {
 // TODO: base operand size?
 public:
  X86Instruction(int size)
    : size_(size) {
  }
  virtual ~X86Instruction() { }
  virtual MachineInstruction assemble() const = 0;
 protected:
  int size() const { return size_; }
  void checkOperandSize(const Operand& a, const Operand& b) const;
 private:
  int size_;
};

using UX86Instruction = std::unique_ptr<X86Instruction>;

// consider using a hierachy for MOV: 
// there are different types / forms of MOV
class Mov : public X86Instruction {
 public:
  Mov(int size, UOperand to, UOperand from)
    : X86Instruction(size),
      to_(std::move(to)),
      from_(std::move(from)) {
    checkOperandSize(*to_, *from_);
  }
  MachineInstruction assemble() const override;
 private:
  UOperand to_;
  UOperand from_;
};

class SysCall : public X86Instruction {
 public:
  SysCall() : X86Instruction(64) { }
  MachineInstruction assemble() const override;
};

}

}
