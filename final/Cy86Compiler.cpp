#include "Cy86Compiler.h"
#include "Cy86Parser.h"

namespace compiler {

using namespace std;
using namespace X86;

#if 0
  unsigned unsigned char code[] = {
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
  return vector<unsigned char>(code, code + sizeof(code));
#endif

namespace {

// TODO: pass this in as parameter
constexpr size_t kStartAddress = 0x400078;

}

std::pair<vector<char>, size_t>
Cy86Compiler::compile(vector<UToken>&& tokens) {
  map<string, size_t> labelToAddress;
  vector<pair<const X86::Immediate*, size_t>> immToFix;

  auto cy86Instructions = Cy86Parser().parse(move(tokens));
  vector<UX86Instruction> x86Instructions;
  for (auto& inst : cy86Instructions) {
    auto x86Insts = inst->translate();
    x86Insts[0]->setLabel(inst->getLabel());
    for (auto& x86Inst : x86Insts) {
      x86Instructions.push_back(move(x86Inst));
    }
  }
  vector<char> code;
  for (auto& inst : x86Instructions) {
    auto machineInst = inst->assemble();
    auto c = machineInst.toBytes();
    for (auto& label : inst->getLabel()) {
      auto ret = labelToAddress.insert(make_pair(label, code.size()));
      if (!ret.second) {
        Throw("Multiple label definition of {}", label);
      }
    }
    code.insert(code.end(), c.begin(), c.end());
    if (auto imm = inst->getImmediateOperand()) {
      if (!imm->getLabel().empty()) {
        immToFix.push_back(make_pair(imm, code.size() - imm->size() / 8));
      }
    }
  }

  for (auto& kv : immToFix) {
    auto imm = kv.first;
    auto loc = kv.second;
    auto it = labelToAddress.find(imm->getLabel());
    if (it == labelToAddress.end()) {
      Throw("label {} undefined", imm->getLabel());
    }

    // get label value (plus base address)
    long value = it->second + kStartAddress;
    // apply constant offset
    value += imm->getConstant();

    auto bytes = ConstantValue::createFundalmentalValue(
                   static_cast<unsigned long>(value))
                     ->toBytes();
    // if the bytes is too long then the rightmost bytes are automatically
    // truncated as desired.
    for (int i = 0; i < imm->size() / 8; ++i) {
      code[loc + i] = bytes[i];
    }
  }

  // TODO: will this perform move?
  auto itStart = labelToAddress.find("start");
  if (itStart != labelToAddress.end()) {
    cout << "start: " << itStart->second + kStartAddress << endl;
  }
  return make_pair(code, itStart == labelToAddress.end() ? 0 : itStart->second);
}

}
