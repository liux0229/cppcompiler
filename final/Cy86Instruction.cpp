#include "Cy86Instruction.h"

namespace compiler {

namespace Cy86 {

using namespace std;

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

class SysCall : Cy86Instruction {
 public:
  SysCall(vector<UOperand>&& operands, int args)
    : Cy86Instruction(move(operands)) {
    if (args + 2 != operands_.size()) {
      Throw("syscall{} requires {}+2 operands; got {}", args, args, operands_);
    }
  }

  vector<UX86Instruction> translate() {
    return {};
  }
};

UCy86Instruction Cy86InstructionFactory::get(const string& opcode,
                                             vector<UOperand>&& operands) {
  if (opcode == "syscall1") {
    return make_unique<SysCall>(move(operands), 1);
  } else {
    Throw("Not supported yet: {}", opcode);
  }
}

}

}
