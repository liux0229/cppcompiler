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

GEN_REG(Rbx, BX, 64)
GEN_REG(Rcx, CX, 64)

#undef GEN_REG

// An immediate can be constructed in two ways
// 1) A list of bytes. This will be used verbatim to form the immediate of
//    the instruction.
// 2) Label + constant + size. The actual content of the immediate is only
//    known after the 1st pass of assembly. Thus size number of zero bytes 
//    are preallocated. (label + constant) computation will be done in the
//    64bit space and potentially truncated to size.
class Immediate : public Operand {
 public:
  Immediate(const std::vector<char> bytes)
    : bytes_(bytes) {
    CHECK(!bytes_.empty());
  }
  Immediate(const std::string& label, int64_t constant, int size) 
    : label_(label),
      constant_(constant),
      size_(size) {
    CHECK(!label_.empty() && size > 0);
  }

  bool isImmediate() const override { return true; }

  // TODO: figure out a better way to represent size
  // # of bits vs. # of bytes
  int size() const override {
    return !bytes_.empty() ? bytes_.size() * 8 : size_;
  }

  std::vector<char> getBytes() const {
    return !bytes_.empty() ? bytes_ : std::vector<char>(size_ / 8, 0);
  }

  const std::string& getLabel() const { return label_; }
  int64_t getConstant() const { return constant_; }

 private:
  const std::vector<char> bytes_;

  const std::string label_;
  int64_t constant_ {0};
  int size_ {0};
};

using UImmediate = std::unique_ptr<Immediate>;

// Assume always use [RDI]; will enhance in the future if necessary
// We currently support two forms of memory addressing:
// RDI and RSP + x (8bit) (using SIB and disp8)
// May enhance to support generic cases
class Memory;
using UMemory = std::unique_ptr<Memory>;

class Memory : public Operand {
  enum Type {
    RDI,
    RSP
  };
 public:
  static UMemory getRdiAddressing(int size) {
    return make_unique<Memory>(size, RDI, 0);
  }
  static UMemory getRspAddressing(int size, char disp) {
    return make_unique<Memory>(size, RSP, disp);
  }
  Memory(int size, Type type, char disp) 
    : size_(size),
      type_(type),
      disp_(disp) { 
  }

  bool isMemory() const override { return true; }
  int size() const override { return size_; }

  bool isRdi() const { return type_ == RDI; }
  bool isRsp() const { return type_ == RSP; }
  char disp() const { 
    CHECK(isRsp());
    return disp_; 
  }
 private:
  int size_;
  Type type_;
  char disp_;
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

  struct SIB {
    unsigned char toByte() const;

    int scale {0};
    int index {0};
    int base {0};
  };

  void addSizePrefix(int size);
  void addRex(Rex r);
  void setModRmRegister(const Register& reg);
  // TODO: consider renaming this setMemory()
  void setModRmMemory(const Memory& mem);
  void setReg(const Register& reg);
  void setReg(unsigned char value);
  void setImmediate(const std::vector<char>& bytes);
  void setRel(unsigned char rel);
  std::vector<unsigned char> toBytes() const;

  void initModRm();
  void initSIB();
  // shared by setModRmRegister && setReg
  void setRegInternal(int& target, const Register& reg, Rex extensionReg);

  std::vector<int> prefix;
  std::vector<Rex> rex;
  std::vector<unsigned char> opcode;
  std::vector<unsigned char> relative;
  std::vector<ModRm> modRm;
  std::vector<SIB> sib;
  std::vector<unsigned char> disp;
  std::vector<unsigned char> immediate;
  bool hi8 {false};
};

inline MachineInstruction::Rex operator|(MachineInstruction::Rex a,
                                         MachineInstruction::Rex b) {
  a |= b;
  return a;
}

class X86Instruction {
 public:
  X86Instruction(int size)
    : size_(size) {
  }
  virtual ~X86Instruction() { }

  void setLabel(const std::vector<std::string>& label) {
    label_ = label;
  }
  const std::vector<std::string>& getLabel() const { return label_; }
  // TODO: is this the best design?
  virtual Immediate* getImmediateOperand() const { return nullptr; }

  virtual MachineInstruction assemble() const = 0;
 protected:
  int size() const { return size_; }
  void checkOperandSize(const Operand& a, const Operand& b) const;
 private:
  int size_;
  std::vector<std::string> label_;
};

using UX86Instruction = std::unique_ptr<X86Instruction>;

// consider using a hierachy for MOV: 
// there are different types / forms of MOV
class Mov : public X86Instruction {
 public:
  Mov(int size, UOperand to, UOperand from);
  MachineInstruction assemble() const override;
  Immediate* getImmediateOperand() const override;
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
  name(int size, UOperand to, UOperand from) \
    : RegRegInstruction(size, std::move(to), std::move(from)) {} \
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
GEN_REG_REG_INST(Test, 0x84, 0x85)

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
  name(int size, UOperand r) : RegInstruction(size, std::move(r)) {} \
  name(int size, UOperand to, UOperand from) : RegInstruction(size, std::move(to), std::move(from)) {} \
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

// TODO: generalize
class JE8 : public X86Instruction {
 public:
  JE8(int relative) 
    : X86Instruction(8),
      relative_(relative) {
  }
  MachineInstruction assemble() const override;
 private:
  int relative_;
};

class JBE8 : public X86Instruction {
 public:
  JBE8(int relative) 
    : X86Instruction(8),
      relative_(relative) {
  }
  MachineInstruction assemble() const override {
    MachineInstruction r;
    r.opcode = {0x76};
    r.setRel(relative_);
    return r;
  }
 private:
  int relative_;
};

#define GEN_SET_INST(name, op) \
class name : public SetInstruction { \
 public: \
  name(UOperand r) : SetInstruction(std::move(r)) {} \
  name(int size, UOperand to, UOperand from) \
    : SetInstruction(size, std::move(to), std::move(from)) {} \
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
// TODO: we need to disable the 8 bit form for JMP / CALL
GEN_REG_INST(JMP,  0xFF, 0xFF, 0x4)
GEN_REG_INST(CALL,  0xFF, 0xFF, 0x2)
GEN_REG_INST(NEG,  0xF6, 0xF7, 0x3)

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

class SingleOpcodeInstruction : public X86Instruction {
 public:
  SingleOpcodeInstruction(const std::vector<unsigned char>& opcode)
    : X86Instruction(64),
      opcode_(opcode) {
  }
  MachineInstruction assemble() const override {
    MachineInstruction r;
    r.opcode = opcode_;
    return r;
  }
 private:
  const std::vector<unsigned char> opcode_;
};

#define GEN_SINGLE_OPCODE_INST(name, ...) \
class name : public SingleOpcodeInstruction { \
 public: \
  name() : SingleOpcodeInstruction(__VA_ARGS__) { } \
};

GEN_SINGLE_OPCODE_INST(SysCall, { 0x0F, 0x05 })
GEN_SINGLE_OPCODE_INST(RET, { 0xC3 })

class SizedOpcodeInstruction : public X86Instruction {
 public:
  SizedOpcodeInstruction(unsigned char opcode, int size)
    : X86Instruction(size),
      opcode_(opcode),
      size_(size) {
  }
  MachineInstruction assemble() const override {
    MachineInstruction r;
    r.addSizePrefix(size_);
    r.opcode = { opcode_ };
    return r;
  }
 private:
  unsigned char opcode_;
  int size_;
};

#define GEN_SIZED_OPCODE_INST(name, opcode, size) \
class name : public SizedOpcodeInstruction { \
 public: \
  name() : SizedOpcodeInstruction(opcode, size) { } \
};


// Sign extend from AX to DX
class SignExtend : public SizedOpcodeInstruction {
 public:
  SignExtend(int size) : SizedOpcodeInstruction(0x99, size) { }
};

GEN_SIZED_OPCODE_INST(CBW, 0x98, 16)
GEN_SIZED_OPCODE_INST(CWDE, 0x98, 32)
GEN_SIZED_OPCODE_INST(CDQE, 0x98, 64)
GEN_SIZED_OPCODE_INST(CWD, 0x99, 16)
GEN_SIZED_OPCODE_INST(CDQ, 0x99, 32)
GEN_SIZED_OPCODE_INST(CQO, 0x99, 64)

#undef GEN_SIZED_OPCODE_INST

class Data : public X86Instruction {
 public:
  Data(UOperand&& imm)
    // TODO: size not real
    : X86Instruction(64),
      imm_(&dynamic_cast<Immediate&>(*imm.release())) {
    CHECK(imm_->getLabel().empty());
  }
  MachineInstruction assemble() const override {
    MachineInstruction r;
    r.setImmediate(imm_->getBytes());
    return r;
  }
 private:
  UImmediate imm_;
};

// TODO: check size constraint
class FLDSTInstruction : public X86Instruction {
 public:
  FLDSTInstruction(int size, 
                   UMemory memory, 
                   int size1, int size2, int size3,
                   unsigned char op1, unsigned char op2, unsigned char op3,
                   unsigned char reg1, unsigned char reg2, unsigned char reg3)
    : X86Instruction(size),
      memory_(std::move(memory)),
      sizes_({size1, size2, size3}),
      opcode_({op1, op2, op3}),
      reg_({reg1, reg2, reg3}) {
  }
  MachineInstruction assemble() const override;
 private:
  UMemory memory_;
  const std::vector<int> sizes_;
  const std::vector<unsigned char> opcode_;
  const std::vector<unsigned char> reg_;
};

#define GEN_FLDST_INST(name, \
                       size1, size2, size3, \
                       op1, op2, op3, \
                       reg1, reg2, reg3) \
class name : public FLDSTInstruction { \
 public: \
  name(int size, UMemory memory) : \
    FLDSTInstruction(size,  \
                     std::move(memory),  \
                     size1, size2, size3, \
                     op1, op2, op3, \
                     reg1, reg2, reg3) { \
  } \
};

GEN_FLDST_INST(FLD, 32, 64, 80, 0xD9, 0xDD, 0xDB, 0x0, 0x0, 0x5);
GEN_FLDST_INST(FSTP, 32, 64, 80, 0xD9, 0xDD, 0xDB, 0x3, 0x3, 0x7);
GEN_FLDST_INST(FILD, 16, 32, 64, 0xDF, 0xDB, 0xDF, 0x0, 0x0, 0x5);
GEN_FLDST_INST(FISTP, 16, 32, 64, 0xDF, 0xDB, 0xDF, 0x3, 0x3, 0x7);

GEN_SINGLE_OPCODE_INST(FADDP, { 0xDE, 0xC1 })
GEN_SINGLE_OPCODE_INST(FSUBP, { 0xDE, 0xE9 })
GEN_SINGLE_OPCODE_INST(FMULP, { 0xDE, 0xC9 })
GEN_SINGLE_OPCODE_INST(FDIVP, { 0xDE, 0xF9 })
// compare ST(0) and ST(1) (0xF0 + 1)
GEN_SINGLE_OPCODE_INST(FCOMIP, { 0xDF, 0xF1 })

#undef GEN_FLDST_INST
#undef GEN_SINGLE_OPCODE_INST


} // X86

} // compiler
