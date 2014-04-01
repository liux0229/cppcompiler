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

const MachineInstruction::Rex MachineInstruction::RexN { false, false,
                                                         false, false, };
const MachineInstruction::Rex MachineInstruction::RexW { true, false,
                                                         false, false, };
const MachineInstruction::Rex MachineInstruction::RexR { false, true,
                                                         false, false };
const MachineInstruction::Rex MachineInstruction::RexX { false, false, 
                                                         true, false };
const MachineInstruction::Rex MachineInstruction::RexB { false, false, 
                                                         false, true };

void MachineInstruction::addSizePrefix(int size) {
  if (size == 16) {
    prefix = { 0x66 };
  } else if (size == 64) {
    addRex(MachineInstruction::RexW);
  }
}

void MachineInstruction::addRex(Rex r) {
  if (rex.empty()) {
    rex.push_back(Rex{});
  }
  rex[0] |= r;
}

void MachineInstruction::setModRmRegister(const Register& reg) {
  initModRm();
  modRm[0].mod = 0x3;
  setRegInternal(modRm[0].rm, reg, RexB);
}

void MachineInstruction::setModRmMemory() {
  initModRm();
  modRm[0].mod = 0x0;
  // TODO: Hard coded to [RDI] for now
  modRm[0].rm = 0x7;
}

void MachineInstruction::setReg(unsigned char value) {
  initModRm();
  modRm[0].reg = value;
}

void MachineInstruction::setReg(const Register& reg) {
  initModRm();
  setRegInternal(modRm[0].reg, reg, RexR);
}

void MachineInstruction::setRegInternal(int& target, 
                                        const Register& reg, 
                                        Rex extensionReg) {
  auto type = reg.getType();
  if (reg.hi8()) {
    hi8 = true;
    target = type + 0x4;
  } else {
    target = type % 0x8;
  }
  if (reg.size() == 8 && (type >= 4 && type < 8)) {
    // SPL, BPL, SDI, DIL
    addRex(RexN);
  }
  if (type >= Register::R8) {
    addRex(extensionReg);
  }
}

// TODO: tighten unsigned and signed char
void MachineInstruction::setImmediate(const vector<char>& bytes) {
  immediate.insert(immediate.end(), bytes.begin(), bytes.end());
}

void MachineInstruction::initModRm() {
  if (modRm.empty()) {
    modRm.push_back(ModRm{});
  }
}

void MachineInstruction::setRel(unsigned char rel) {
  relative = { rel };
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
  if (hi8 && !rex.empty()) {
    Throw("AH/BH/CH/DH addressed in an instruction requiring REX");
  }
  vector<unsigned char> r;
  if (!prefix.empty()) {
    r.push_back(prefix[0]);
  }
  if (!rex.empty()) {
    r.push_back(rex[0].toByte());
  }
  r.insert(r.end(), opcode.begin(), opcode.end());
  if (!relative.empty()) {
    r.push_back(relative[0]);
  }
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

const Immediate* Mov::getImmediateOperand() const {
  if (from_->isImmediate()) {
    return &toImmediate(*from_);
  } else {
    return nullptr;
  }
}

// TODO: consider a design where opcode, mod/rm, reg fields are obtained
// separately
MachineInstruction Mov::assemble() const {
  // TODO: handle 80 moves (can only use floating points)
  MachineInstruction r;
  r.addSizePrefix(size());

  if (to_->isRegister() && from_->isRegister()) {
    r.opcode = { size() == 8 ? 0x88 : 0x89 };
    r.setModRmRegister(toRegister(*to_));
    r.setReg(toRegister(*from_));
  } else if (to_->isRegister() && from_->isImmediate()) {
    auto type = toRegister(*to_).getType();
    r.opcode = { static_cast<unsigned char>(
                   (size() == 8 ? 0xB0 : 0xB8) + type % 0x8) 
               };
    if (type >= Register::R8) {
      r.addRex(MachineInstruction::RexB);
    }
    auto& from = toImmediate(*from_);

    // imm does not contain label reference, can directly generate
    // machine code
    if (from.label().empty()) {
      r.setImmediate(from.getLiteral()->toBytes());
    } else {
      // generate place holder
      r.setImmediate(vector<char>(size(), 0));
    }

  } else if (to_->isRegister() && from_->isMemory()) {
    r.opcode = { size() == 8 ? 0x8A : 0x8B };
    r.setReg(toRegister(*to_));
    r.setModRmMemory();
  } else if (to_->isMemory() && from_->isRegister()) {
    r.opcode = { size() == 8 ? 0x88 : 0x89 };
    r.setModRmMemory();
    r.setReg(toRegister(*from_));
  } else {
    MCHECK(false, "Mov: invalid operand combination");
  }

  return r;
}

RegRegInstruction::RegRegInstruction(int size, UOperand to, UOperand from) 
  : X86Instruction(size) {
  if (!to->isRegister() || !from->isRegister()) {
    Throw("RegRegInstruction requires operands to be registers");
  }
  to_ = URegister(static_cast<Register*>(to.release()));
  from_ = URegister(static_cast<Register*>(from.release()));
}

MachineInstruction RegRegInstruction::assemble() const {
  MachineInstruction r;
  r.addSizePrefix(size());
  auto opcode = getOpcode();
  r.opcode.insert(r.opcode.end(), opcode.begin(), opcode.end());
  r.setModRmRegister(*to_);
  r.setReg(*from_);
  return r;
}

RegInstruction::RegInstruction(int size,UOperand reg)
  : X86Instruction(size) {
  if (!reg->isRegister()) {
    Throw("RegInstruction requires operand to be register");
  }
  reg_ = URegister(static_cast<Register*>(reg.release()));
}

RegInstruction::RegInstruction(int size, UOperand to, UOperand from) 
  : X86Instruction(size) {
  if (!to->isRegister() || !from->isRegister()) {
    Throw("RegInstruction requires operands to be registers");
  }
  auto toReg = static_cast<Register*>(to.get());
  if (toReg->getType() != Register::AX) {
    Throw("To operand of RegInstruction must be AX, got {}", toReg->getType());
  }
  reg_ = URegister(static_cast<Register*>(from.release()));
}

MachineInstruction RegInstruction::assemble() const {
  MachineInstruction r;
  r.addSizePrefix(size());
  auto opcode = getOpcode();
  r.opcode.insert(r.opcode.end(), opcode.begin(), opcode.end());
  r.setModRmRegister(*reg_);
  r.setReg(getReg());
  return r;
}

MachineInstruction JE8::assemble() const {
  MachineInstruction r;
  r.opcode = {0x74};
  r.setRel(relative_);
  return r;
}

} // X86

} // compiler
