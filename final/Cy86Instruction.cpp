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
    { dx, X86::Register::DX },
    { di, X86::Register::DI },
    { si, X86::Register::SI },
    { r8, X86::Register::R8 },
    { r9, X86::Register::R9 },
    { r10, X86::Register::R10 }
  };
  return make_unique<X86::Register>(m.at(type_), size_);
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
    { dx, "DX" },
    { di, "DI" },
    { si, "SI" },
    { r8, "R8" },
    { r9, "R9" },
    { r10, "R10" }
  };
  out << format("{}{}", m.at(type_), size_);
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

bool Cy86InstructionFactory::match(const string& target, 
                                   const string& opcode, 
                                   int& size) {
  auto it = find_if(opcode.begin(), 
                    opcode.end(), 
                    [](char x) { return isdigit(x); });
  if (it == opcode.end()) {
    return false;
  }
  size = stoi(opcode.substr(it - opcode.begin()));

  set<int> allowedSize {
    8, 16, 32, 64, 80
  };
  if (allowedSize.find(size) == allowedSize.end()) {
    return false;
  }

  return target == opcode.substr(0, it - opcode.begin());
}

UCy86Instruction Cy86InstructionFactory::get(const string& opcode,
                                             vector<UOperand>&& operands) {
  int size;
  if (match("move", opcode, size)) {
    return make_unique<Mov>(size, move(operands));
  } else if (opcode.substr(0, opcode.size() - 1) == "syscall") {
    int args = opcode[opcode.size() - 1] - '0';
    if (args < 0 || args > 6) {
      Throw("Bad opcode: {}", opcode);
    }
    return make_unique<SysCall>(args, move(operands));
  } else {
    Throw("Bad opcode: {}", opcode);
    return nullptr;
  }
}

} // Cy86

} // compiler
