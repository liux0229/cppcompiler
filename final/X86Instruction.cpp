// TODO: add ADD support so we can complete the memory mode support
#include "X86Instruction.h"

namespace compiler {

namespace X86 {

using namespace std;

namespace {

const Register& toRegister(const Operand& operand) {
  auto p = dynamic_cast<const Register*>(&operand);
  MCHECK(p, "register operand expected");
  return *p;
}

const Immediate& toImmediate(const Operand& operand) {
  auto p = dynamic_cast<const Immediate*>(&operand);
  MCHECK(p, "register operand expected");
  return *p;
}

} // unnamed

const MachineInstruction::Rex MachineInstruction::RexW { true, false,
                                                         false, false, };
const MachineInstruction::Rex MachineInstruction::RexR { false, true,
                                                         false, false };
const MachineInstruction::Rex MachineInstruction::RexX { false, false, 
                                                         true, false };
const MachineInstruction::Rex MachineInstruction::RexB { false, false, 
                                                         false, true };

void MachineInstruction::addRex(Rex r) {
  if (rex.empty()) {
    rex.push_back(Rex{});
  }
  rex[0] |= r;
}

void MachineInstruction::setModRmRegister(Register::Type type) {
  initModRm();
  modRm[0].mod = 0x3;
  modRm[0].rm = type % 0x8;
  if (type >= Register::R8) {
    rex[0].B = 1;
  }
}

void MachineInstruction::setModRmMemory() {
  initModRm();
  modRm[0].mod = 0x0;
  // TODO: Hard coded to [RDI] for now
  modRm[0].rm = 0x7;
}

void MachineInstruction::setReg(Register::Type type) {
  initModRm();
  modRm[0].reg = type % 0x8;
  if (type >= Register::R8) {
    rex[0].R = 1;
  }
}

void MachineInstruction::setImmediate(SConstantValue imm) {
  auto b = imm->toBytes();
  immediate.insert(immediate.end(), b.begin(), b.end());
}

void MachineInstruction::initModRm() {
  if (modRm.empty()) {
    modRm.push_back(ModRm{});
  }
}

unsigned char MachineInstruction::Rex::toByte() const {
  unsigned char r = 0;
  r |= 0x40;
  r |= W << 3;
  r |= R << 2;
  r |= X << 1;
  r |= B << 0;
  return r;
}

unsigned char MachineInstruction::ModRm::toByte() const {
  unsigned char r = 0;
  r |= rm;
  r |= reg << 3;
  r |= mod << 6;
  return r;
}

vector<unsigned char> MachineInstruction::toBytes() const {
  vector<unsigned char> r;
  if (!rex.empty()) {
    r.push_back(rex[0].toByte());
  }
  r.insert(r.end(), opcode.begin(), opcode.end());
  if (!modRm.empty()) {
    r.push_back(modRm[0].toByte());
  }
  r.insert(r.end(), immediate.begin(), immediate.end());
  return r;
}

void X86Instruction::checkOperandSize(const Operand& a, 
                                      const Operand& b) const {
  MCHECK(a.size() == b.size(), 
         format("x86 operand size mismatch: {} vs {}",
                a.size(),
                b.size()));
}

// TODO: consider a design where opcode, mod/rm, reg fields are obtained
// separately
MachineInstruction Mov::assemble() const {
  // TODO: handle 80 moves (can only use floating points)
  MachineInstruction r;
  if (size() == 64) {
    r.addRex(MachineInstruction::RexW);
  }

  if (to_->isRegister() && from_->isRegister()) {
    r.opcode = { size() == 8 ? 0x88 : 0x89 };
    r.setModRmRegister(toRegister(*to_).getType());
    r.setReg(toRegister(*from_).getType());
  } else if (to_->isRegister() && from_->isImmediate()) {
    auto type = toRegister(*to_).getType();
    r.opcode = { static_cast<unsigned char>(
                   (size() == 8 ? 0xB0 : 0xB8) + type % 0x8) 
               };
    if (type >= Register::R8) {
      r.addRex(MachineInstruction::RexB);
    }
    auto& from = toImmediate(*from_);
    r.setImmediate(from.getLiteral());
  } else if (to_->isRegister() && from_->isMemory()) {
    r.opcode = { size() == 8 ? 0x8A : 0x8B };
    r.setReg(toRegister(*to_).getType());
    r.setModRmMemory();
  } else if (to_->isMemory() && from_->isRegister()) {
    r.opcode = { size() == 8 ? 0x88 : 0x89 };
    r.setModRmMemory();
    r.setReg(toRegister(*from_).getType());
  } else {
    MCHECK(false, "Mov: invalid operand combination");
  }

  return r;
}

MachineInstruction SysCall::assemble() const {
  MachineInstruction r;
  r.opcode = { 0x0F, 0x05 };
  return r;
}

} // X86

} // compiler
