#include "Cy86Compiler.h"
#include "Cy86Parser.h"

namespace compiler {

using namespace std;

#if 0
  unsigned char code[] = {
    // ==== write(stdout, "TODO\n") ====
    // mov rax, 1 ... system call `write`
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00,

    // mov rdi, 1 ... stdout fd
    0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00,

    // mov rsi, 0x400000 + 64 + 56 ... address of "TODO" string
    /*
       0x48, 0xbe,
       0x78, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
     */
    0x48, 0xc7, 0xc6, 0x78, 0x00, 0x40, 0x00,

    // mov rdx, 28 ... num bytes to write
    0x48, 0xc7, 0xc2, 35, 0x00, 0x00, 0x00,

    // syscall
    0x0f, 0x05,
    // =====================


    // ===== exit(0) =======
    // mov rax, 60 ... system call `exit`
    0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00,

    // mov rdi, 0 ... exit status 0
    0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00,

    // syscall
    0x0f, 0x05
      // =====================
  };
  return vector<char>(code, code + sizeof(code));
#endif

vector<char> Cy86Compiler::compile(vector<UToken>&& tokens) {
  auto cy86Instructions = Cy86Parser().parse(move(tokens));
  // May need a semantic analysis pass (e.g. tigthen up label references)
  vector<UX86Instruction> x86Instructions;
  for (auto& inst : cy86Instructions) {
    auto x86Insts = inst->translate();
    for (auto& x86Inst : x86Insts) {
      x86Instructions.push_back(move(x86Inst));
    }
  }
  vector<char> code;
  for (auto& inst : x86Instructions) {
    auto c = inst->assemble();
    code.insert(code.end(), c.begin(), c.end());
  }
  return code;
}

}
