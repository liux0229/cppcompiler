// Implementation plan:
// All integer operations
// Full memory addressing support
// Label support
// Enhance operand size/type check (for operand sign, can also allow
//                                  "interpreting as unsigned, for example)
// Floating point support

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
  Register(Type type, int size, bool hi8 = false)
    : type_(type),
      size_(size),
      hi8_(hi8) {
    if (hi8_) {
      MCHECK(size_ == 8 && (type_ < 4), 
             format("Invalid x86 register specifier: t:{} s:{} h8:{}",
                    type_, size_, hi8_));
    }
  }

  bool isRegister() const override { return true; }

  int size() const override { return size_; }

  Type getType() const {
    return type_;
  }
  bool hi8() const {
    return hi8_;
  }
 private:
  Type type_;
  int size_;
  bool hi8_;
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
  static const Rex RexN; // none of WRXB bits set
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

  void addSizePrefix(int size);
  void addRex(Rex r);
  void setModRmRegister(const Register& reg);
  void setModRmMemory();
  void setReg(const Register& reg);
  void setReg(unsigned char value);
  void setImmediate(SConstantValue imm);
  std::vector<unsigned char> toBytes() const;

  void initModRm();
  // shared by setModRmRegister && setReg
  void setRegInternal(int& target, const Register& reg, Rex extensionReg);

  std::vector<int> prefix;
  std::vector<Rex> rex;
  std::vector<unsigned char> opcode;
  std::vector<ModRm> modRm;
  std::vector<unsigned char> immediate;
  bool hi8 {false};
};

inline MachineInstruction::Rex operator|(MachineInstruction::Rex a,
                                         MachineInstruction::Rex b) {
  a |= b;
  return a;
}

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

class RegRegInstruction : public X86Instruction {
 public:
  // To be easily brought into the public section of its derived classes
  RegRegInstruction(int size, UOperand to, UOperand from);
  MachineInstruction assemble() const override;
 protected:
  virtual std::vector<int> getOpcode() const = 0;
 private:
  URegister to_;
  URegister from_;
};

#define GEN_REG_REG_INST(name, op8, op32) \
class name : public RegRegInstruction { \
 public: \
  using RegRegInstruction::RegRegInstruction; \
  std::vector<int> getOpcode() const override { \
    return { size() == 8 ? op8 : op32 }; \
  } \
};

GEN_REG_REG_INST(Add, 0x0, 0x1)
GEN_REG_REG_INST(Sub, 0x28, 0x29)
GEN_REG_REG_INST(And, 0x20, 0x21)
GEN_REG_REG_INST(Or, 0x08, 0x09)
GEN_REG_REG_INST(Xor, 0x30, 0x31)
GEN_REG_REG_INST(Cmp, 0x3A, 0x3B)

#undef GEN_REG_REG_INST

class RegInstruction : public X86Instruction {
 public:
  // TODO: consider whether this two constructor form can be improved
  RegInstruction(int size, UOperand reg);
  RegInstruction(int size, UOperand to, UOperand from);

  MachineInstruction assemble() const override;
 protected:
  virtual std::vector<int> getOpcode() const = 0;
  virtual int getReg() const = 0; 
 private:
  URegister reg_;
};

#define GEN_REG_INST(name, op8, op32, reg) \
class name : public RegInstruction { \
 public: \
  using RegInstruction::RegInstruction; \
 protected: \
  std::vector<int> getOpcode() const override { \
    return { size() == 8 ? op8 : op32 }; \
  } \
  int getReg() const override { \
    return reg; \
  } \
};

class SetInstruction : public RegInstruction {
 public:
  SetInstruction(UOperand reg)
    : RegInstruction(8, move(reg)) {
  }

  // Needed because it's used in the addOperation virtual function of
  // BinaryOperation; we could avoid this by moving its definition to a
  // derived class.
  SetInstruction(int size, UOperand to, UOperand from) 
    : RegInstruction(size, std::move(to), std::move(from)) {
    Throw("Not implemented");
  }
 protected:
  std::vector<int> getOpcode() const override {
    std::vector<int> opcode;
    opcode.push_back(0x0F);
    opcode.push_back(getOpcodeInternal());
    return opcode;
  }
  virtual int getOpcodeInternal() const = 0;
  int getReg() const override {
    return 0;
  }
};


#define GEN_SET_INST(name, op) \
class name : public SetInstruction { \
 public: \
  using SetInstruction::SetInstruction; \
 protected: \
  int getOpcodeInternal() const override { \
    return op; \
  } \
};

GEN_REG_INST(Not,  0xF6, 0xF7, 0x2)
GEN_REG_INST(UMul, 0xF6, 0xF7, 0x4)
GEN_REG_INST(SMul, 0xF6, 0xF7, 0x5)
GEN_REG_INST(UDiv, 0xF6, 0xF7, 0x6)
GEN_REG_INST(SDiv, 0xF6, 0xF7, 0x7)
GEN_REG_INST(UMod, 0xF6, 0xF7, 0x6)
GEN_REG_INST(SMod, 0xF6, 0xF7, 0x7)
GEN_REG_INST(SHL,  0xD2, 0xD3, 0x4)
GEN_REG_INST(SHR,  0xD2, 0xD3, 0x5)
GEN_REG_INST(SAR,  0xD2, 0xD3, 0x7)

GEN_SET_INST(SETA, 0x97)
GEN_SET_INST(SETAE, 0x93)
GEN_SET_INST(SETB, 0x92)
GEN_SET_INST(SETBE, 0x96)
GEN_SET_INST(SETG, 0x9F)
GEN_SET_INST(SETGE, 0x9D)
GEN_SET_INST(SETL, 0x9C)
GEN_SET_INST(SETLE, 0x9E)
GEN_SET_INST(SETE, 0x94)
GEN_SET_INST(SETNE, 0x95)

#undef GEN_REG_INST

class SysCall : public X86Instruction {
 public:
  SysCall() : X86Instruction(64) { }
  MachineInstruction assemble() const override;
};

}

}
