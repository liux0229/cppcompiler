#include "Cy86Instruction.h"
#include <cctype>
#include <algorithm>
#include <utility>

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

// TODO; consider supporting a variadic template form
vector<UOperand> combine(UOperand&& a, UOperand&& b) {
  vector<UOperand> r;
  r.push_back(move(a));
  r.push_back(move(b));
  return r;
}

bool match(const string& opcode, 
           const string& target, 
           int maxSize, 
           int& size) {
  auto it = find_if(opcode.begin(), 
                    opcode.end(), 
                    [](char x) { return isdigit(x); });
  if (it == opcode.end()) {
    return false;
  }
  size = stoi(opcode.substr(it - opcode.begin()));

  set<int> allowedSize {
    8, 16, 32, 64
  };
  if (maxSize == 80) {
    allowedSize.insert(80);
  }
  if (allowedSize.find(size) == allowedSize.end()) {
    return false;
  }

  return target == opcode.substr(0, it - opcode.begin());
}

template<typename T>
UCy86Instruction match(const string& opcode,
                       vector<UOperand>&& operands,
                       const string& target, 
                       int maxSize) {
  int size;
  if (match(opcode, target, maxSize, size)) {
    return make_unique<T>(size, move(operands));
  } else {
    return nullptr;
  }
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
    { r10, X86::Register::R10 }
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
    { r10, "R10" }
  };
  out << format("{}{} h8:{}", m.at(type_), size_, hi8_);
}

X86::UOperand Immediate::toX86Operand(int size) const {
  auto type = literal_->type;
  if (!type->isFundalmental()) {
    Throw("Literal type not supported yet: {}", type);
  }
  auto ftype = type->toFundalmental();
  if (!ftype->isIntegral()) {
    Throw("Literal type not supported yet: {}", ftype);
  }

  EFundamentalType target;
  if (size == 8) {
    target = FT_CHAR;
  } else if (size == 16) {
    target = FT_SHORT_INT;
  } else if (size == 32) {
    target = FT_INT;
  } else if (size == 64) {
    target = FT_LONG_INT;
  } else {
    Throw("Unsupported target size: {}", size);
  }
  return make_unique<X86::Immediate>(literal_->to(target));
}

void Immediate::output(std::ostream& out) const {
  // TODO: better outputing for constant value
  out << "<immediate>";
}

void Memory::output(std::ostream& out) const {
  out << format("[{} + <literal>]", *reg_);
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
    // TODO: handle size-80
    vector<UX86Instruction> r;
    if (operands()[0]->isRegister() || operands()[1]->isRegister()) {
      if (operands()[1]->isMemory()) {
        Memory* m = static_cast<Memory*>(operands()[1].get());
        // TODO: enhance this support after we have ADD
        // TODO: In theory we could accept a sub-64 register operand;
        // however this matches the reference implementation
        add<X86::Mov>(r, 64, X86::Rdi(), m->reg()->toX86Operand(64));
        add<X86::Mov>(r, 
                      size(),
                      operands()[0]->toX86Operand(size()), 
                      make_unique<X86::Memory>(size()));
      } else if (operands()[0]->isMemory()) {
        Memory* m = static_cast<Memory*>(operands()[0].get());
        // TODO: enhance this support after we have ADD
        add<X86::Mov>(r, 64, X86::Rdi(), m->reg()->toX86Operand(64));
        add<X86::Mov>(r, 
                      size(),
                      make_unique<X86::Memory>(size()),
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
    unsigned char jmpSize = jmp->assemble().toBytes().size();

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
  DivOperation(const string& name, int size, vector<UOperand>&& operands)
    : BinaryOperation<TX86Instruction>(name,
                                       size,
                                       move(operands)) {
  }
  void addOperation(vector<UX86Instruction>& r) const override {
    // This looks like a compiler bug
    auto sz = Cy86Instruction::size();

    // Clear AH/DX
    auto reg = sz == 8 ? Register::Ah() : Register::Dx(sz);
    add<X86::Xor>(r,
                  sz,
                  reg->toX86Operand(sz),
                  reg->toX86Operand(sz));
                                              
    // AX = (DX:)AX div BX
    add<TX86Instruction>(r,
                         sz,
                         Register::Ax(sz)->toX86Operand(sz),
                         Register::Bx(sz)->toX86Operand(sz));
  }
};

template<typename TX86Instruction>
class ModOperation : public DivOperation<TX86Instruction> {
 protected:
  ModOperation(const string& name, int size, vector<UOperand>&& operands)
    : DivOperation<TX86Instruction>(name,
                                    size,
                                    move(operands)) {
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
                    Immediate(make_shared<FundalmentalValue<char>>(FT_CHAR, 8))
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

GEN_BINARY_OP_WITH_BASE(UDiv, DivOperation)
GEN_BINARY_OP_WITH_BASE(SDiv, DivOperation)
GEN_BINARY_OP_WITH_BASE(UMod, ModOperation)
GEN_BINARY_OP_WITH_BASE(SMod, ModOperation)

GEN_BINARY_OP_WITH_BASE(SHL, ShiftOperation)
GEN_BINARY_OP_WITH_BASE(SAR, ShiftOperation)
GEN_BINARY_OP_WITH_BASE(SHR, ShiftOperation)

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
#undef GEN_BINARY_OP_WITH_BASE

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
    args.push_back(Register::Rdi());
    args.push_back(Register::Rsi());
    args.push_back(Register::Rdx());
    args.push_back(Register::R10());
    args.push_back(Register::R8());
    args.push_back(Register::R9());

    for (size_t i = 2; i < operands().size(); ++i) {
      auto inst = Mov(64, move(args[i - 2]), move(operands()[i])).translate();
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
  }

  UCy86Instruction ret;
  (ret = match<Mov>(opcode, move(operands), "move", 64)) ||
  (ret = match<Add>(opcode, move(operands), "iadd", 64)) ||
  (ret = match<Sub>(opcode, move(operands), "isub", 64)) ||
  (ret = match<UMul>(opcode, move(operands), "umul", 64)) ||
  (ret = match<SMul>(opcode, move(operands), "smul", 64)) ||
  (ret = match<UDiv>(opcode, move(operands), "udiv", 64)) ||
  (ret = match<SDiv>(opcode, move(operands), "sdiv", 64)) ||
  (ret = match<UMod>(opcode, move(operands), "umod", 64)) ||
  (ret = match<SMod>(opcode, move(operands), "smod", 64)) ||
  (ret = match<Not>(opcode, move(operands), "not", 64)) ||
  (ret = match<And>(opcode, move(operands), "and", 64)) ||
  (ret = match<Or>(opcode, move(operands), "or", 64)) ||
  (ret = match<Xor>(opcode, move(operands), "xor", 64)) ||
  (ret = match<SHL>(opcode, move(operands), "lshift", 64)) ||
  (ret = match<SAR>(opcode, move(operands), "srshift", 64)) ||
  (ret = match<SHR>(opcode, move(operands), "urshift", 64)) ||
  (ret = match<CMPE>(opcode, move(operands), "ieq", 64)) ||
  (ret = match<CMPNE>(opcode, move(operands), "ine", 64)) ||
  (ret = match<CMPL>(opcode, move(operands), "slt", 64)) ||
  (ret = match<CMPB>(opcode, move(operands), "ult", 64)) ||
  (ret = match<CMPG>(opcode, move(operands), "sgt", 64)) ||
  (ret = match<CMPA>(opcode, move(operands), "ugt", 64)) ||
  (ret = match<CMPLE>(opcode, move(operands), "sle", 64)) ||
  (ret = match<CMPBE>(opcode, move(operands), "ule", 64)) ||
  (ret = match<CMPGE>(opcode, move(operands), "sge", 64)) ||
  (ret = match<CMPAE>(opcode, move(operands), "uge", 64));

  if (!ret) {
    Throw("Bad opcode: {}", opcode);
  }
  return ret;
}

} // Cy86

} // compiler
