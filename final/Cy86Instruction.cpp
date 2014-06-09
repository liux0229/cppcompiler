// TODO: register + literal (literal must be constant)
#include "Cy86Instruction.h"
#include <cctype>
#include <algorithm>
#include <utility>
#include <set>

namespace compiler {

namespace Cy86 {

using namespace std;
using X86::UX86Instruction;

namespace {

template<typename TInstruction, typename... Args>
void add(vector<UX86Instruction>& r, Args... args) {
  r.push_back(make_unique<TInstruction>(forward<Args>(args)...));
}

void add(vector<UX86Instruction>& r, vector<UX86Instruction>&& from) {
  for (auto& s : from) {
    r.push_back(move(s));
  }
}

void combineInternal(vector<UOperand>& r) {
}

template<typename... T>
void combineInternal(vector<UOperand>& r, UOperand car, T... cdr) {
  r.push_back(move(car));
  combineInternal(r, move(cdr)...);
}

template<typename... T>
vector<UOperand> combine(T... op) {
  vector<UOperand> r;
  combineInternal(r, move(op)...);
  return r;
}

bool match(const string& opcode,
           const string& targetPattern, 
           const set<int>& allowedSize, 
           int& size) {
  for (auto s : allowedSize) {
    // TODO: fix this
    if (format(targetPattern.c_str(), s) == opcode) {
      size = s;
      return true;
    }
  }
  return false;
}

template<typename T>
UCy86Instruction match(const string& opcode,
                       vector<UOperand>&& operands,
                       const string& target, 
                       const set<int>& allowedSize) {
  int size;
  if (match(opcode, target, allowedSize, size)) {
    return make_unique<T>(size, move(operands));
  } else {
    return nullptr;
  }
}

Register* toRegister(const UOperand& op) {
  return static_cast<Register*>(op.get());
}

Memory* toMemory(const UOperand& op) {
  return static_cast<Memory*>(op.get());
}

vector<char> truncateOrExtend(SConstantValue literal, 
                              int desiredSizeInBits) {
  auto bytes = literal->toBytes();
  int actualSize = static_cast<int>(bytes.size());
  int desiredSize = static_cast<int>(desiredSizeInBits / 8);
  if (actualSize >= desiredSize) {
    bytes = vector<char>(bytes.begin(), bytes.begin() + desiredSize);
  } else {
    // only sign extend if the literal is a integral signed type
    auto type = literal->type;
    bool extend = type->isFundalmental() && 
                  type->toFundalmental()->isIntegral() &&
                  static_cast<FundalmentalValueBase*>(literal.get())
                    ->isSigned();
    char extendByte = extend && ((bytes.back() & 0x80) > 0) ? 
                        0xFF :
                        0x00;
    for (int i = 0; i < desiredSize - actualSize; ++i) {
      bytes.push_back(extendByte);
    }
  }
  return bytes;
}

void setupMemoryOperand(vector<UX86Instruction>& r, Memory* m) {
  // note: we require registers to be 64
  if (m->reg()) {
    add<X86::Mov>(r, 64, X86::Rdi(), m->reg()->toX86Operand(64));
  } else {
    add<X86::Xor>(r, 64, X86::Rdi(), X86::Rdi());
  }
  if (m->imm()) {
    add<X86::Mov>(r, 64, X86::Rsi(), m->imm()->toX86Operand(64));
  } else {
    add<X86::Xor>(r, 64, X86::Rsi(), X86::Rsi());
  }
  add<X86::Add>(r, 64, X86::Rdi(), X86::Rsi());
}

void moveImmediateToRspMemory(vector<UX86Instruction>& r,
                              int size,
                              char rspDisp,
                              X86::UImmediate&& imm) {
  add<X86::Mov>(r, 
                size,
                // TODO: this is a bit cumbersome
                Register::Ax(size)->toX86Operand(size),
                move(imm));
  add<X86::Mov>(r, 
                size,
                X86::Memory::getRspAddressing(size, rspDisp),
                Register::Ax(size)->toX86Operand(size));
}

SConstantValue getConvOffset(int size) {
  long offset = 1L << (size - 1);
  if (size == 8) {
    return ConstantValue::createFundalmentalValue(static_cast<char>(offset));
  } else if (size == 16) {
    return ConstantValue::createFundalmentalValue(static_cast<short>(offset));
  } else if (size == 32) {
    return ConstantValue::createFundalmentalValue(static_cast<int>(offset));
  } else if (size == 64) {
    return ConstantValue::createFundalmentalValue(offset);
  }
  CHECK(false);
  return nullptr;
}

}

Register::Register(const string& name) {
  if (name == "sp") {
    set(SP, 64);
  } else if (name == "bp") {
    set(BP, 64);
  } else {
    int s = stoi(name.substr(1));
    switch (name[0]) {
      case 'x':
        set(X, s);
        break;
      case 'y':
        set(Y, s);
        break;
      case 'z':
        set(Z, s);
        break;
      case 't':
        set(T, s);
        break;
    }
  }
}

X86::UOperand Register::toX86Operand(int size) const {
  if (size_ != size) {
    Throw("Operation requires {} sized register operand; got {}",
          size,
          *this);
  }
  map<Type, X86::Register::Type> m {
    { BP, X86::Register::BP },
    { SP, X86::Register::SP },
    { X, X86::Register::R12 },
    { Y, X86::Register::R13 },
    { Z, X86::Register::R14 },
    { T, X86::Register::R15 },
    { ax, X86::Register::AX },
    { bx, X86::Register::BX },
    { cx, X86::Register::CX },
    { dx, X86::Register::DX },
    { di, X86::Register::DI },
    { si, X86::Register::SI },
    { r8, X86::Register::R8 },
    { r9, X86::Register::R9 },
    { r10, X86::Register::R10 },
    { r11, X86::Register::R11 }
  };
  return make_unique<X86::Register>(m.at(type_), size_, hi8_);
}

void Register::output(std::ostream& out) const {
  map<Type, string> m {
    { BP, "BP" },
    { SP, "SP" },
    { X, "X" },
    { Y, "Y" },
    { Z, "Z" },
    { T, "T" },
    { ax, "AX" },
    { bx, "BX" },
    { cx, "CX" },
    { dx, "DX" },
    { di, "DI" },
    { si, "SI" },
    { r8, "R8" },
    { r9, "R9" },
    { r10, "R10" },
    { r11, "R11" }
  };
  out << format("{}{} h8:{}", m.at(type_), size_, hi8_);
}

X86::UOperand Immediate::toX86Operand(int size) const {
  // no literal, must be plain label
  if (!literal_) {
    CHECK(!label_.empty());
    return make_unique<X86::Immediate>(label_, 0, size);
  }

  // immediate with no size restriction
  // pass verbatim
  if (size == -1) {
    return make_unique<X86::Immediate>(literal_->toBytes());
  }

  if (!label_.empty()) {
    auto type = literal_->type;
    bool isIntegral = type->isFundalmental() &&
                      type->toFundalmental()->isIntegral();
    if (!isIntegral) {
      Throw("Immediate with label {} but constant type is not integral: {}",
            label_,
            *type);
    }
    auto bytes = truncateOrExtend(literal_, 64);
    CHECK(bytes.size() == 8);
    // TODO: abstract this
    auto constant = *reinterpret_cast<int64_t *>(bytes.data());
    return make_unique<X86::Immediate>(label_, constant, size);
  } else {
    return make_unique<X86::Immediate>(truncateOrExtend(literal_, size));
  }
}

void Immediate::output(std::ostream& out) const {
  // TODO: better outputing for constant value
  out << format("<{} {}>", label_, literal_ ? "immediate" : "");
}

void Memory::output(std::ostream& out) const {
  out << "[";
  if (reg_ && !imm_) {
    out << format("{}", *reg_);
  } else if (!reg_ && imm_) {
    out << format("{}", *imm_);
  } else if (reg_ && imm_) {
    out << format("{} + {}", *reg_, *imm_);
  } else {
    CHECK(false);
  }
  out << "]";
}

void Cy86Instruction::checkWriteable(const string& name) const {
  if (!operands()[0]->writeable()) {
    Throw("First operand to {} must be writeable, got: {}", 
          name,
          *operands()[0]);
  }
}

void Cy86Instruction::checkOperandNumber(const string& name, size_t n) const {
  if (n != operands().size()) {
    Throw("{} requires {} operands; got {}", 
          name,
          n,
          operands().size());
  }
}

class FLD : public Cy86Instruction {
 public:
  FLD(int size, UOperand operand) 
    : Cy86Instruction(size, combine(move(operand))) {
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    auto& op = operands()[0];
    if (op->isMemory()) {
      setupMemoryOperand(r, toMemory(op));
      add<X86::FLD>(r, size(), X86::Memory::getRdiAddressing(size()));
    } else if (op->isRegister()) {
      // move register to [RSP - 16] and then FLD
      add<X86::Mov>(r, 
                    size(), 
                    X86::Memory::getRspAddressing(size(), -16),
                    op->toX86Operand(size()));
      add<X86::FLD>(r, 
                    size(), 
                    X86::Memory::getRspAddressing(size(), -16));
    } else {
      CHECK(op->isImmediate());
      // imm. For size 32 and 64, move it to RSP - 16 and FLD.
      //      For size 80, move lower 64 to RSP - 16, higher 16 to RSP - 8,
      //                   and FLD.
      //      If there is a label involved, then the label always goes 
      //      with the lower bytes, since the higher 16 bytes (if applicable)
      //      would always be 0.
      // note we are hand crafting the X86 instructions instead of using
      // cy86 intermediate since we don't have enough machinary to express
      // RSP addressing and the bytes <-> immediate conversion is also
      // cumbersome. can consider redesign.
      auto x86Imm = X86::UImmediate(
                      static_cast<X86::Immediate*>(
                        op->toX86Operand(size()).release()));
      int lowerSize = min(64, size());
      X86::UImmediate lowerImm;
      if (!x86Imm->getLabel().empty()) {
        lowerImm = make_unique<X86::Immediate>(
                     x86Imm->getLabel(),
                     x86Imm->getConstant(), 
                     lowerSize);
      } else {
        auto bytes = x86Imm->getBytes();
        lowerImm = make_unique<X86::Immediate>(
                     vector<char>(bytes.begin(), 
                                  bytes.begin() + lowerSize / 8));
      }
      moveImmediateToRspMemory(r, lowerSize, -16, move(lowerImm));

      if (size() == 80) {
        vector<char> higherBytes;
        if (!x86Imm->getLabel().empty()) {
          higherBytes = vector<char>(2, 0x0);
        } else {
          auto bytes = x86Imm->getBytes();
          CHECK(bytes.size() == 10);
          higherBytes = vector<char>(bytes.begin() + 8, bytes.end());
        }
        moveImmediateToRspMemory(r, 
                                 16, 
                                 -8,
                                 make_unique<X86::Immediate>(higherBytes));
      }
      
      add<X86::FLD>(r, 
                    size(), 
                    X86::Memory::getRspAddressing(size(), -16));
    }
    
    return r;
  }
};

class FST : public Cy86Instruction {
 public:
  FST(int size, UOperand operand) 
    : Cy86Instruction(size, combine(move(operand))) {
    checkWriteable("FSTInstruction");
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    auto& op = operands()[0];
    if (op->isMemory()) {
      setupMemoryOperand(r, toMemory(op));
      add<X86::FSTP>(r, size(), X86::Memory::getRdiAddressing(size()));
    } else if (op->isRegister()) {
      // FST to [RSP - 16] and then to register
      add<X86::FSTP>(r, 
                     size(), 
                     X86::Memory::getRspAddressing(size(), -16));
      add<X86::Mov>(r, 
                    size(), 
                    op->toX86Operand(size()),
                    X86::Memory::getRspAddressing(size(), -16));
    }
    return r;
  }
};

class Mov : public Cy86Instruction {
 public:
  Mov(int size, vector<UOperand>&& ops)
    : Cy86Instruction(size, move(ops)) {
    checkOperandNumber("Mov", 2);
    checkWriteable("Mov");
  }
  Mov(int size, UOperand to, UOperand from)
    : Mov(size, combine(move(to), move(from))) {
  }

  vector<UX86Instruction> translate() {
    if (size() < 80) {
      return smallSizeMove();
    } else {
      return largeSizeMove();
    }
  }

  vector<UX86Instruction> smallSizeMove() {
    vector<UX86Instruction> r;
    if (operands()[0]->isRegister() || operands()[1]->isRegister()) {
      if (operands()[1]->isMemory()) {
        setupMemoryOperand(r, toMemory(operands()[1]));
        add<X86::Mov>(r, 
                      size(),
                      operands()[0]->toX86Operand(size()), 
                      X86::Memory::getRdiAddressing(size()));
      } else if (operands()[0]->isMemory()) {
        setupMemoryOperand(r, toMemory(operands()[0]));
        add<X86::Mov>(r, 
                      size(),
                      X86::Memory::getRdiAddressing(size()),
                      operands()[1]->toX86Operand(size()));
      } else {
        add<X86::Mov>(r, 
                      size(), 
                      operands()[0]->toX86Operand(size()),
                      operands()[1]->toX86Operand(size()));
      }
    } else {
      {
        auto inst = Mov(size(), Register::Ax(size()), move(operands()[1]))
                      .translate();
        add(r, move(inst));
      }
      {
        auto inst = Mov(size(), move(operands()[0]), Register::Ax(size()))
                      .translate();
        add(r, move(inst));
      }
    }

    return r;
  }

  vector<UX86Instruction> largeSizeMove() {
    auto& to = operands()[0];
    auto& from = operands()[1];

    vector<UX86Instruction> r;
    {
      auto inst = FLD(80, move(from)).translate();
      add(r, move(inst));
    }
    {
      auto inst = FST(80, move(to)).translate();
      add(r, move(inst));
    }
    return r;
  }
};

template<typename TX86Instruction>
class UnaryOperation : public Cy86Instruction {
  vector<UX86Instruction> translate() override {
    vector<UX86Instruction> r;
    // move operand 1 into AX
    {
      auto inst = Mov(size(), Register::Ax(size()), move(operands()[1]))
                    .translate();
      add(r, move(inst));
    }

    // AX = op AX
    add<TX86Instruction>(r, size(), Register::Ax(size())->toX86Operand(size()));

    {
      auto inst = Mov(size(), move(operands()[0]), Register::Ax(size()))
                    .translate();
      add(r, move(inst));
    }

    return r;
  }
 protected:
  UnaryOperation(const string& name, 
                 int size, 
                 vector<UOperand>&& ops)
    : Cy86Instruction(size, move(ops)) {
    checkOperandNumber(name, 2);
    checkWriteable(name);
  }
};

template<typename TX86Instruction>
class ControlTransferInstruction : public Cy86Instruction {
 public:
  ControlTransferInstruction(const string& name, vector<UOperand>&& ops)
    : Cy86Instruction(64, move(ops)) {
    checkOperandNumber(name, 1);
  }
  vector<UX86Instruction> translate() override {
    vector<UX86Instruction> r;
    // move operand 0 into AX
    {
      auto inst = Mov(size(), Register::Ax(size()), move(operands()[0]))
                    .translate();
      add(r, move(inst));
    }

    add<TX86Instruction>(r, 64, Register::Ax(64)->toX86Operand(64));

    return r;
  }
};

class RET : public Cy86Instruction {
 public:
  RET(vector<UOperand>&& ops) 
    : Cy86Instruction(64, move(ops)) {
    checkOperandNumber("RET", 0);
  }
  vector<UX86Instruction> translate() override {
    vector<UX86Instruction> r;
    add<X86::RET>(r);
    return r;
  }
};

#define GEN_CONTROL_TRANSFER_OP(name) \
class name : public ControlTransferInstruction<X86::name> { \
 public: \
  name(vector<UOperand>&& ops) \
    : ControlTransferInstruction(#name, move(ops)) { \
  } \
};

GEN_CONTROL_TRANSFER_OP(JMP)
GEN_CONTROL_TRANSFER_OP(CALL)

#undef GEN_CONTROL_TRANSFER_OP

class JumpIf : public Cy86Instruction {
 public:
  JumpIf(vector<UOperand>&& ops)
    : Cy86Instruction(64, move(ops)) {
    checkOperandNumber("JumpIf", 2);
  }
  vector<UX86Instruction> translate() override {
    vector<UX86Instruction> r;
    // move operand 1 into RBX
    {
      auto inst = Mov(64, Register::Rbx(), move(operands()[1])).translate();
      add(r, move(inst));
    }
    // move operand 0 into AL
    {
      auto inst = Mov(8, Register::Ax(8), move(operands()[0])).translate();
      add(r, move(inst));
    }
    // Test AL
    add<X86::Test>(r,
                   8, 
                   Register::Ax(8)->toX86Operand(8),
                   Register::Ax(8)->toX86Operand(8));

    // actual JMP
    auto jmp = make_unique<X86::JMP>(64, Register::Rbx()->toX86Operand(64));
    // TODO: is this legit?
    unsigned char jmpSize = static_cast<unsigned char>(jmp->assemble().toBytes().size());

    // if the !operand(0), then do not perform (skip) the actual JMP
    add<X86::JE8>(r, jmpSize);

    // add JMP to the instruction stream
    r.push_back(move(jmp));

    return r;
  }
};

// TODO; note that the gcc compiler does not give very good error messages
// if I omit "public" by mistake (the unique_ptr assignment will fail)
template<typename TX86Instruction>
class BinaryOperation : public Cy86Instruction {
 public:
  vector<UX86Instruction> translate() override {
    vector<UX86Instruction> r;
    // move operand 1 into AX
    {
      auto inst = Mov(size(), Register::Ax(size()), move(operands()[1]))
                    .translate();
      add(r, move(inst));
    }

    storeOperand2(r);

    // perform operation
    addOperation(r);

    // move result into operand 0
    storeResult(r);

    return r;
  }
 protected:
  BinaryOperation(const string& name, 
                  int size, 
                  vector<UOperand>&& ops)
    : Cy86Instruction(size, move(ops)) {
    checkOperandNumber(name, 3);
    checkWriteable(name);
  }

  virtual void storeOperand2(vector<UX86Instruction>& r) {
    // move operand 2 into BX
    auto inst = Mov(size(), Register::Bx(size()), move(operands()[2]))
                  .translate();
    add(r, move(inst));
  }

  virtual void addOperation(vector<UX86Instruction>& r) const {
    // AX = AX op BX
    add<TX86Instruction>(r, 
                         size(),
                         Register::Ax(size())->toX86Operand(size()), 
                         Register::Bx(size())->toX86Operand(size()));
  }

  virtual void storeResult(vector<UX86Instruction>& r) {
    auto inst = Mov(size(), move(operands()[0]), Register::Ax(size()))
                  .translate();
    add(r, move(inst));
  }
};

template<typename TX86Instruction>
class DivOperation : public BinaryOperation<TX86Instruction> {
 protected:
  DivOperation(const string& name, 
               int size, 
               vector<UOperand>&& operands, 
               bool isSigned)
    : BinaryOperation<TX86Instruction>(name,
                                       size,
                                       move(operands)),
      isSigned_(isSigned) {
  }
  void addOperation(vector<UX86Instruction>& r) const override {
    // This looks like a compiler bug
    auto sz = Cy86Instruction::size();

    if (isSigned_) {
      // Sign extend to AX / DX
      if (sz == 8) {
        add<X86::CBW>(r);
      } else {
        add<X86::SignExtend>(r, sz);
      }
    } else {
      auto reg = sz == 8 ? Register::Ah() : Register::Dx(sz);
      add<X86::Xor>(r,
          sz,
          reg->toX86Operand(sz),
          reg->toX86Operand(sz));
    }

    // AX = (DX:)AX div BX
    add<TX86Instruction>(r,
                         sz,
                         Register::Ax(sz)->toX86Operand(sz),
                         Register::Bx(sz)->toX86Operand(sz));
  }
 private:
  bool isSigned_;
};

template<typename TX86Instruction>
class ModOperation : public DivOperation<TX86Instruction> {
 protected:
  ModOperation(const string& name, 
               int size, 
               vector<UOperand>&& operands, 
               bool isSigned)
    : DivOperation<TX86Instruction>(name,
                                    size,
                                    move(operands),
                                    isSigned) {
  }
  void storeResult(vector<UX86Instruction>& r) override {
    auto sz = Cy86Instruction::size();
    auto& ops = Cy86Instruction::operands();

    auto reg = sz == 8 ? Register::Ah() : Register::Dx(sz);
    if (reg->hi8()) {
      // note we cannot rely on intermediate instruction translation since
      // we cannot clobber AX; at this point we must hand pick the instructions
      add<X86::Mov>(r, 
                    8, 
                    Register::Cx(8)->toX86Operand(8), 
                    Immediate("", 
                              make_shared<FundalmentalValue<char>>(FT_CHAR, 8))
                      .toX86Operand(8));
      add<X86::SHR>(r, 16, Register::Ax(16)->toX86Operand(16));
      reg = reg->toLower();
    }
    {
      auto inst = Mov(sz, move(ops[0]), move(reg)).translate();
      add(r, move(inst));
    }
  }
};

template<typename TX86Instruction>
class ShiftOperation : public BinaryOperation<TX86Instruction> {
 protected:
  ShiftOperation(const string& name, int size, vector<UOperand>&& operands)
    : BinaryOperation<TX86Instruction>(name,
                                       size,
                                       move(operands)) {
  }
  virtual void storeOperand2(vector<UX86Instruction>& r) {
    // move operand 2 into CL
    auto& ops = Cy86Instruction::operands();
    auto inst = Mov(8, Register::Cx(8), move(ops[2])).translate();
    add(r, move(inst));
  }
  void addOperation(vector<UX86Instruction>& r) const override {
    auto sz = Cy86Instruction::size();
    add<TX86Instruction>(r, sz, Register::Ax(sz)->toX86Operand(sz));
  }
};

template<typename TX86Instruction>
class CompareOperation : public BinaryOperation<TX86Instruction> {
 protected:
  CompareOperation(const string& name, int size, vector<UOperand>&& operands)
    : BinaryOperation<TX86Instruction>(name,
                                       size,
                                       move(operands)) {
  }

  void addOperation(vector<UX86Instruction>& r) const override {
    auto sz = Cy86Instruction::size();
    add<X86::Cmp>(r, 
                  sz, 
                  Register::Bx(sz)->toX86Operand(sz),
                  Register::Ax(sz)->toX86Operand(sz));
    add<TX86Instruction>(r, Register::Ax(8)->toX86Operand(8));
  }
  void storeResult(vector<UX86Instruction>& r) override {
    auto inst = Mov(8, move(Cy86Instruction::operands()[0]), Register::Ax(8))
                  .translate();
    add(r, move(inst));
  }
};

#define GEN_UNARY_OP(name) \
class name : public UnaryOperation<X86::name> { \
 public: \
  name(int size, vector<UOperand>&& operands) \
    : UnaryOperation<X86::name>(#name, size, move(operands)) { \
  } \
};

#define GEN_BINARY_OP_WITH_BASE(name, base) \
class name : public base<X86::name> { \
 public: \
  name(int size, vector<UOperand>&& operands) \
    : base<X86::name>(#name, size, move(operands)) { \
  } \
};

#define GEN_BINARY_OP_WITH_BASE_SIGN(name, base, isSigned) \
class name : public base<X86::name> { \
 public: \
  name(int size, vector<UOperand>&& operands) \
    : base<X86::name>(#name, size, move(operands), isSigned) { \
  } \
};

#define GEN_COMPARE_OP(name) \
class CMP ## name : public CompareOperation<X86::SET ## name> { \
 public: \
  CMP ## name(int size, vector<UOperand>&& operands) \
    : CompareOperation<X86::SET ## name>("CMP" #name, size, move(operands)) { \
  } \
};

#define GEN_BINARY_OP(name) GEN_BINARY_OP_WITH_BASE(name, BinaryOperation)

GEN_UNARY_OP(Not)

GEN_BINARY_OP(Add)
GEN_BINARY_OP(Sub)
GEN_BINARY_OP(UMul)
GEN_BINARY_OP(SMul)
GEN_BINARY_OP(And)
GEN_BINARY_OP(Or)
GEN_BINARY_OP(Xor)

GEN_BINARY_OP_WITH_BASE(SHL, ShiftOperation)
GEN_BINARY_OP_WITH_BASE(SAR, ShiftOperation)
GEN_BINARY_OP_WITH_BASE(SHR, ShiftOperation)

GEN_BINARY_OP_WITH_BASE_SIGN(UDiv, DivOperation, false)
GEN_BINARY_OP_WITH_BASE_SIGN(SDiv, DivOperation, true)
GEN_BINARY_OP_WITH_BASE_SIGN(UMod, ModOperation, false)
GEN_BINARY_OP_WITH_BASE_SIGN(SMod, ModOperation, true)

GEN_COMPARE_OP(A)
GEN_COMPARE_OP(AE)
GEN_COMPARE_OP(B)
GEN_COMPARE_OP(BE)
GEN_COMPARE_OP(G)
GEN_COMPARE_OP(GE)
GEN_COMPARE_OP(L)
GEN_COMPARE_OP(LE)
GEN_COMPARE_OP(E)
GEN_COMPARE_OP(NE)

#undef GEN_COMPARE_OP
#undef GEN_BINARY_OP
#undef GEN_BINARY_OP_WITH_BASE_SIGN
#undef GEN_BINARY_OP_WITH_BASE

class Data : public Cy86Instruction {
 public:
  Data(int size, vector<UOperand>&& ops) 
    : Cy86Instruction(size, move(ops)) {
    checkOperandNumber("data", 1);
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    add<X86::Data>(r, operands()[0]->toX86Operand(size()));
    return r;
  }
};


class SysCall : public Cy86Instruction {
 public:
  SysCall(size_t args, vector<UOperand>&& ops)
    : Cy86Instruction(64, move(ops)) {
    checkOperandNumber(format("syscall{}", args), args + 2);
    checkWriteable("syscall");
  }

  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    {
      auto inst = Mov(64, Register::Rax(), move(operands()[1])).translate();
      add(r, move(inst));
    }

    vector<UOperand> args;
    // Note: Rdi / Rsi need to be specially handled since they are used 
    // in [memory] addressing (so they could be clobbered)
    // The plan is to first move operand 2 & 3 to R11 and RCX
    // Then after all the operands are moved move R11/RCX to Rdi/Rsi 
    args.push_back(Register::R11());
    args.push_back(Register::Rcx());

    args.push_back(Register::Rdx());
    args.push_back(Register::R10());
    args.push_back(Register::R8());
    args.push_back(Register::R9());

    for (size_t i = 2; i < operands().size(); ++i) {
      auto inst = Mov(64, move(args[i - 2]), move(operands()[i])).translate();
      add(r, move(inst));
    }

    {
      auto inst = Mov(64, Register::Rdi(), Register::R11()).translate();
      add(r, move(inst));
    }
    {
      auto inst = Mov(64, Register::Rsi(), Register::Rcx()).translate();
      add(r, move(inst));
    }

    add<X86::SysCall>(r);
    {
      auto inst = Mov(64, move(operands()[0]), Register::Rax()).translate();
      add(r, move(inst));
    }

    return r;
  }
};

template<typename TX86Instruction>
class FArithmeticOperation : public Cy86Instruction {
 public:
  // TODO: consider variadic template parameter?
  FArithmeticOperation(const string& name, 
                         int size, 
                         vector<UOperand>&& operands) 
    : Cy86Instruction(size, move(operands)) {
    checkOperandNumber(name, 3);
    checkWriteable(name);
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    {
      auto inst = FLD(size(), move(operands()[1])).translate();
      add(r, move(inst));
    }
    {
      auto inst = FLD(size(), move(operands()[2])).translate();
      add(r, move(inst));
    }
    add<TX86Instruction>(r);
    {
      auto inst = FST(size(), move(operands()[0])).translate();
      add(r, move(inst));
    }

    return r;
  }
};

#define GEN_FARITH_OP(name) \
class name : public FArithmeticOperation<X86::name ## P> { \
 public: \
  name(int size, vector<UOperand>&& operands) \
    : FArithmeticOperation<X86::name ## P>(#name, size, move(operands)) { \
  } \
};

GEN_FARITH_OP(FADD)
GEN_FARITH_OP(FSUB)
GEN_FARITH_OP(FMUL)
GEN_FARITH_OP(FDIV)

#undef GEN_FARITH_OP

template<typename TX86Instruction>
class FCompareOperation : public Cy86Instruction {
 protected:
  FCompareOperation(const string& name, int size, vector<UOperand>&& operands)
    : Cy86Instruction(size, move(operands)) {
    checkOperandNumber(name, 3);
    checkWriteable(name);
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    {
      auto inst = FLD(size(), move(operands()[2])).translate();
      add(r, move(inst));
    }
    {
      auto inst = FLD(size(), move(operands()[1])).translate();
      add(r, move(inst));
    }

    add<X86::FCOMIP>(r);
    // save the comparison result to AL
    add<TX86Instruction>(r, Register::Ax(8)->toX86Operand(8));

    {
      auto inst = Mov(8, move(Cy86Instruction::operands()[0]), Register::Ax(8))
                    .translate();
      add(r, move(inst));
    }

    // pop the FPU stack
    add<X86::FSTP>(r, 64, X86::Memory::getRspAddressing(64, -16));

    return r;
  }
};

#define GEN_FCMP_OP(name) \
class FCMP ## name : public FCompareOperation<X86::SET ## name> { \
 public: \
  FCMP ## name(int size, vector<UOperand>&& operands)  \
    : FCompareOperation<X86::SET ## name>("FCMP" #name, \
                                          size, \
                                          move(operands)) { \
  } \
};

GEN_FCMP_OP(A)
GEN_FCMP_OP(AE)
GEN_FCMP_OP(B)
GEN_FCMP_OP(BE)
GEN_FCMP_OP(E)
GEN_FCMP_OP(NE)

#undef GEN_FCMP_OP

template<bool IsSigned>
class FILDInstruction : public Cy86Instruction {
 public:
  FILDInstruction(int size, UOperand operand) 
    : Cy86Instruction(size, combine(move(operand))) {
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;

    // because we need to support 8 bit source, we need to first move the source
    // into AX, then [RSP - 16], then FILD
    {
      auto inst = Mov(size(), Register::Ax(size()), move(operands()[0]))
             .translate();
      add(r, move(inst));
    }
    if (size() == 8) {
      if (IsSigned) {
        // Sign extend to AX
        add<X86::CBW>(r);
      } else {
        add<X86::Xor>(
            r, 
            8, 
            Register::Ah()->toX86Operand(8),
            Register::Ah()->toX86Operand(8));
      }
    }
    int actualSize = max(16, size());
    add<X86::Mov>(r, 
                  actualSize, 
                  X86::Memory::getRspAddressing(actualSize, -16),
                  Register::Ax(actualSize)->toX86Operand(actualSize));
    add<X86::FILD>(r,  
                   actualSize, 
                   X86::Memory::getRspAddressing(actualSize, -16));
    
    return r;
  }
};

using FULD = FILDInstruction<false>;
using FSLD = FILDInstruction<true>;

class FIST : public Cy86Instruction {
 public:
  FIST(int size, UOperand operand) 
    : Cy86Instruction(size, combine(move(operand))) {
    checkWriteable("FIST");
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;

    // FIST to [RSP - 16], move (truncate) to Ax, and then to the dest
    add<X86::FISTP>(r, 64, X86::Memory::getRspAddressing(64, -16));
    add<X86::Mov>(r, 
                  size(),
                  Register::Ax(size())->toX86Operand(size()), 
                  X86::Memory::getRspAddressing(size(), -16));
    {
      auto inst = Mov(size(), move(operands()[0]), Register::Ax(size()))
                    .translate();
      add(r, move(inst));
    }

    return r;
  }
};

template<typename TFLD, typename TFST, bool IFrom80>
class ConvOperation : public Cy86Instruction {
 public:
  ConvOperation(int size, vector<UOperand>&& operands)
    : Cy86Instruction(size, move(operands)) {
    checkOperandNumber("ConvOperation", 2);
    checkWriteable("ConvOperation");
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    {
      auto inst = TFLD(IFrom80 ? 80 : size(), move(operands()[1])).translate();
      add(r, move(inst));
    }
    {
      bool to80 = !IFrom80;
      auto inst = TFST(to80 ? 80 : size(), move(operands()[0])).translate();
      add(r, move(inst));
    }
    return r;
  }
};

using ConvF80F = ConvOperation<FLD, FST, true>;
using ConvFF80 = ConvOperation<FLD, FST, false>;
using ConvF80S = ConvOperation<FLD, FIST, true>;
using ConvSF80 = ConvOperation<FSLD, FST, false>;

// TODO: alternative memory management for operands
class ConvUF80 : public Cy86Instruction {
 public:
  ConvUF80(int size, vector<UOperand>&& operands) 
    : Cy86Instruction(size, move(operands)) {
  }
  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;

    add<X86::Xor>(r, 64, X86::Rcx(), X86::Rcx());
    {
      auto inst = Mov(size(), Register::Cx(size()), move(operands()[1]))
                    .translate();
      add(r, move(inst));
    }
    {
      auto inst = FULD(64, Register::Rcx()).translate();
      add(r, move(inst));
    }
    {
      auto constant = ConstantValue::createFundalmentalValue(1ULL << 63);
      auto inst = Mov(64, 
                      Register::Rbx(), 
                      make_unique<Immediate>("", constant)).translate();
      add(r, move(inst));
    }
    add<X86::Cmp>(r, 64, X86::Rbx(), X86::Rcx());

    size_t start = r.size();

    // Now we lay out the instructions to adjust the final result
    {
      auto constant 
             = ConstantValue::createFundalmentalValue(18446744073709551616.0);
      auto inst = FLD(64, make_unique<Immediate>("", constant)).translate();
      add(r, move(inst));
    }
    add<X86::FADDP>(r);

    // Now generate the jump
    int offset = 0;
    for (size_t i = start; i < r.size(); ++i) {
      offset += static_cast<int>(r[i]->assemble().toBytes().size());
    }
    CHECK(offset < 127);

    auto jmp = make_unique<X86::JBE8>(offset);
    r.insert(r.begin() + start, move(jmp));

    // finally, the store
    {
      auto inst = FST(80, move(operands()[0])).translate();
      add(r, move(inst));
    }

    return r;
  }
};

class ConvF80U : public Cy86Instruction {
 public:
  ConvF80U(int size, vector<UOperand>&& operands) 
    : Cy86Instruction(size, move(operands)) {
  }

  vector<UX86Instruction> translate() {
    vector<UX86Instruction> r;
    auto constant = ConstantValue::createFundalmentalValue(1ULL << 63);

    {
      auto inst = FLD(80, move(operands()[1])).translate();
      add(r, move(inst));
    }
    {
      auto inst = FSLD(64, make_unique<Immediate>("", constant)).translate();
      add(r, move(inst));
    }
    add<X86::FADDP>(r);

    {
      auto inst = FIST(64, Register::Rcx()).translate();
      add(r, move(inst));
    }
    {
      auto inst = Mov(64, Register::Rbx(), make_unique<Immediate>("", constant))
                    .translate();
      add(r, move(inst));
    }
    add<X86::Add>(r, 64, X86::Rcx(), X86::Rbx());

    {
      auto inst = Mov(size(), move(operands()[0]), Register::Cx(size()))
                    .translate();
      add(r, move(inst));
    }

    return r;
  }
};

UCy86Instruction Cy86InstructionFactory::get(const string& opcode,
                                             vector<UOperand>&& operands) {
  if (opcode.substr(0, opcode.size() - 1) == "syscall") {
    int args = opcode[opcode.size() - 1] - '0';
    if (args < 0 || args > 6) {
      Throw("Bad opcode: {}", opcode);
    }
    // TODO: maybe check # args here
    return make_unique<SysCall>(args, move(operands));
  } else if (opcode == "jump") {
    return make_unique<JMP>(move(operands));
  } else if (opcode == "jumpif") {
    return make_unique<JumpIf>(move(operands));
  } else if (opcode == "call") {
    return make_unique<CALL>(move(operands));
  } else if (opcode == "ret") {
    return make_unique<RET>(move(operands));
  } else if (opcode == "datax") {
    return make_unique<Data>(-1, move(operands));
  }

  UCy86Instruction ret;
  (ret = match<Mov>(opcode, move(operands), "move{}", {8,16,32,64, 80})) ||
  (ret = match<Add>(opcode, move(operands), "iadd{}", {8,16,32,64})) ||
  (ret = match<Sub>(opcode, move(operands), "isub{}", {8,16,32,64})) ||
  (ret = match<UMul>(opcode, move(operands), "umul{}", {8,16,32,64})) ||
  (ret = match<SMul>(opcode, move(operands), "smul{}", {8,16,32,64})) ||
  (ret = match<UDiv>(opcode, move(operands), "udiv{}", {8,16,32,64})) ||
  (ret = match<SDiv>(opcode, move(operands), "sdiv{}", {8,16,32,64})) ||
  (ret = match<UMod>(opcode, move(operands), "umod{}", {8,16,32,64})) ||
  (ret = match<SMod>(opcode, move(operands), "smod{}", {8,16,32,64})) ||
  (ret = match<Not>(opcode, move(operands), "not{}", {8,16,32,64})) ||
  (ret = match<And>(opcode, move(operands), "and{}", {8,16,32,64})) ||
  (ret = match<Or>(opcode, move(operands), "or{}", {8,16,32,64})) ||
  (ret = match<Xor>(opcode, move(operands), "xor{}", {8,16,32,64})) ||
  (ret = match<SHL>(opcode, move(operands), "lshift{}", {8,16,32,64})) ||
  (ret = match<SAR>(opcode, move(operands), "srshift{}", {8,16,32,64})) ||
  (ret = match<SHR>(opcode, move(operands), "urshift{}", {8,16,32,64})) ||
  (ret = match<CMPE>(opcode, move(operands), "ieq{}", {8,16,32,64})) ||
  (ret = match<CMPNE>(opcode, move(operands), "ine{}", {8,16,32,64})) ||
  (ret = match<CMPL>(opcode, move(operands), "slt{}", {8,16,32,64})) ||
  (ret = match<CMPB>(opcode, move(operands), "ult{}", {8,16,32,64})) ||
  (ret = match<CMPG>(opcode, move(operands), "sgt{}", {8,16,32,64})) ||
  (ret = match<CMPA>(opcode, move(operands), "ugt{}", {8,16,32,64})) ||
  (ret = match<CMPLE>(opcode, move(operands), "sle{}", {8,16,32,64})) ||
  (ret = match<CMPBE>(opcode, move(operands), "ule{}", {8,16,32,64})) ||
  (ret = match<CMPGE>(opcode, move(operands), "sge{}", {8,16,32,64})) ||
  (ret = match<CMPAE>(opcode, move(operands), "uge{}", {8,16,32,64})) ||
  (ret = match<Data>(opcode, move(operands), "data{}", {8,16,32,64})) ||
  (ret = match<FADD>(opcode, move(operands), "fadd{}", {32,64,80})) ||
  (ret = match<FSUB>(opcode, move(operands), "fsub{}", {32,64,80})) ||
  (ret = match<FMUL>(opcode, move(operands), "fmul{}", {32,64,80})) ||
  (ret = match<FDIV>(opcode, move(operands), "fdiv{}", {32,64,80})) ||
  (ret = match<FCMPE>(opcode, move(operands), "feq{}", {32,64,80})) ||
  (ret = match<FCMPNE>(opcode, move(operands), "fne{}", {32,64,80})) ||
  (ret = match<FCMPB>(opcode, move(operands), "flt{}", {32,64,80})) ||
  (ret = match<FCMPBE>(opcode, move(operands), "fle{}", {32,64,80})) ||
  (ret = match<FCMPA>(opcode, move(operands), "fgt{}", {32,64,80})) ||
  (ret = match<FCMPAE>(opcode, move(operands), "fge{}", {32,64,80})) ||
  (ret = match<ConvF80S>(opcode, move(operands), "f80convs{}", {8,16,32,64})) ||
  (ret = match<ConvF80U>(opcode, move(operands), "f80convu{}", {8,16,32,64})) ||
  (ret = match<ConvF80F>(opcode, move(operands), "f80convf{}", {32,64})) ||
  (ret = match<ConvSF80>(opcode, move(operands), "s{}convf80", {8,16,32,64})) ||
  (ret = match<ConvUF80>(opcode, move(operands), "u{}convf80", {8,16,32,64})) ||
  (ret = match<ConvFF80>(opcode, move(operands), "f{}convf80", {32,64}));

  if (!ret) {
    Throw("Bad opcode: {}", opcode);
  }
  return ret;
}

} // Cy86

} // compiler
