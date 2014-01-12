#include "Linker.h"

#include <tuple>

namespace compiler {

using namespace std;

namespace {

enum CompareResult {
  Conflict,
  MultipleDefs,
  Same,
  Replace,
  New,
};

CompareResult compareVariable(SVariableMember e, SVariableMember m) {
  bool replace = false;
  if (*e->type != *m->type) {
    bool bad = true;
    if (e->type->isArray() && m->type->isArray()) {
      auto& ea = static_cast<ArrayType&>(*e->type);
      auto& ma = static_cast<ArrayType&>(*m->type);
      if (ea.addSizeTo(ma)) {
        bad = false;
      } else if (ma.addSizeTo(ea)) {
        bad = false;
        replace = true;
      }
    }
    if (bad) {
      return Conflict;
    }
  }

  if (e->isDefined && m->isDefined) {
    return MultipleDefs;
  }

  if (replace || m->isDefined) {
    return Replace;
  }

  return Same;
}

CompareResult compareFunction(SFunctionMember e, SFunctionMember m) {
  if (*e->type == *m->type) {
    if (e->isDefined && m->isDefined) {
      return MultipleDefs;
    } else if (m->isDefined) {
      return Replace;
    } else {
      return Same;
    }
  } else if (e->type->sameParameterAndQualifier(*m->type)) {
    return Conflict;
  } else {
    return New;
  }
}

CompareResult compareEntity(SMember e, SMember m) {
  if (e->getKind() != m->getKind()) {
    return Conflict;
  }
  return m->isVariable() ?
           compareVariable(e->toVariable(), m->toVariable()) :
           compareFunction(e->toFunction(), m->toFunction());
}

} // unnamed

using Address = Linker::Address;

void Linker::addTranslationUnit(UTranslationUnit&& unit) {
  units_.push_back(move(unit));
}

void Linker::addExternal(SMember m) {
  auto name = m->getQualifiedName();
  auto p = members_.equal_range(name);
  for (auto it = p.first; it != p.second; ++it) {
    auto result = compareEntity(it->second, m);
    switch (result) {
      case Conflict:
        Throw("conflicting declarations for {}: {} vs {}", 
              name, 
              *it->second, 
              *m);
        break;
      case MultipleDefs:
        Throw("Multiple definitions of {}", *m);
        break;
      case Replace:
        it->second = m;
        return;
      case Same:
        return;
      case New:
        break;
    }
  }

  members_.insert(make_pair(name, m));
}

SMember Linker::getExternal(SMember m) {
  auto name = m->getQualifiedName();
  auto p = members_.equal_range(name);
  for (auto it = p.first; it != p.second; ++it) {
    auto ret = compareEntity(it->second, m);
    if (ret == MultipleDefs || ret == Same) {
      return it->second;
    }
  }
  MCHECK(false, format("cannot find external member: {}", name));
  return nullptr;
}

SMember Linker::getUnique(SMember m) {
  if (m->linkage != Linkage::External) {
    return m;
  } else {
    return getExternal(m);
  }
}

void Linker::addLiteral(const ConstantValue* literal, Address addr) {
  auto ret = addressToLiteral_.insert(
               make_pair(literal,
                         vector<Address>()));
  if (ret.second) {
    literals_.push_back(literal);
  }

  if (addr.valid()) {
    ret.first->second.push_back(addr);
  }
}

void Linker::addTemporary(SVariableMember m, Address addr) {
  auto ret = addressToTemporary_.insert(
               make_pair(m,
                         vector<Address>()));
  if (ret.second) {
    temporary_.push_back(m);
  }

  CHECK(addr.valid());
  ret.first->second.push_back(addr);
}

void Linker::checkOdr() {
  for (auto& u : units_) {
    auto& ms = u->getVariablesFunctions();
    for (auto& m : ms) {
      if (m->linkage != Linkage::External) {
        if (!m->isDefined) {
          Throw("{} is not defined", *m);
        }
      } else {
        addExternal(m);
      }

      if (m->isDefined) {
        cout << "Linking: " << *m;
        if (m->isVariable()) {
          cout << " " << *m->toVariable()->initializer;
        }
        cout << endl;
      }
    }
  }
  for (auto& kv : members_) {
    if (!kv.second->isDefined) {
      Throw("{} not defined", *kv.second);
    }
  }
}


vector<char> Linker::getAddress(size_t addr) {
  return ConstantValue::createFundalmentalValue(
           FT_UNSIGNED_LONG_INT,
           static_cast<unsigned long>(addr))
         ->toBytes();
}

bool Linker::getAddress(SMember m, vector<char>& ret) {
  m = getUnique(m);
  auto it = memberAddress_.find(m);
  if (it == memberAddress_.end()) {
    return false;
  }
  auto addr = it->second;
  ret = getAddress(addr.first);
  return true;
}

void Linker::update(Address target, const vector<char>& bytes) {
  CHECK(target.second - target.first == bytes.size());
  for (size_t i = 0; i < bytes.size(); ++i) {
    image_[target.first + i] = bytes[i];
  }
}

Address Linker::genEntry(const char* data, size_t n, size_t alignment) {
  auto start = ((image_.size() + alignment - 1) / alignment) * alignment;
  while (image_.size() < start) {
    image_.push_back(0);
  }
  image_.insert(image_.end(), data, data + n);
  return Address(image_.end() - image_.begin() - n, 
                 image_.end() - image_.begin()); 
}


void Linker::genHeader() {
  gen("PA8");
}

Address Linker::genFunction() {
  return gen("fun");
}

Address Linker::genZero(size_t n, size_t alignment) {
  vector<char> v(n);
  return genEntry(v.data(), n, alignment); 
}

// TODO: consider making image generation part of Type so we can use
// virtual dispatch
Address Linker::genFundalmental(SVariableMember m) {
  CHECK(m->initializer);
  auto& initializer = *m->initializer;
  if (!initializer.isDefault() && initializer.expr->isConstant()) {
    auto literal = initializer.expr->toConstant();
    auto bytes = literal->value->toBytes();
    return genEntry(bytes.data(), bytes.size(), m->type->getTypeAlign());
  } else {
    return genZero(m->type->getTypeSize(), m->type->getTypeAlign());
  }
}

Address Linker::genArray(SVariableMember m) {
  CHECK(m->initializer);
  auto& initializer = *m->initializer;
  if (initializer.isDefault()) {
    return genZero(m->type->getTypeSize(), m->type->getTypeAlign());
  } else {
    CHECK(initializer.expr->isConstant());
    auto literalExpr = initializer.expr->toConstant();
    addLiteral(literalExpr->value.get());

    auto bytes = literalExpr->value->toBytes();
    auto addr1 = genEntry(bytes.data(), bytes.size(), m->type->getTypeAlign());
    auto addr2 = genZero(m->type->getTypeSize() - bytes.size(), 
                         m->type->getTypeAlign());
    return Address(addr1.first, addr2.second);
  }
}

Address Linker::genPointer(SVariableMember m) {
  CHECK(m->initializer);
  auto& initializer = *m->initializer;
  if (!initializer.isDefault() && initializer.expr->isConstant()) {
    auto literal = initializer.expr->toConstant();
    auto addr = genZero(m->type->getTypeSize(), m->type->getTypeAlign());

    auto value = literal->value.get();
    if (auto ma = dynamic_cast<MemberAddressValue*>(value)) {
      vector<char> bytes;
      if (getAddress(ma->member, bytes)) {
        update(addr, bytes);
      } else {
        addressToMember_[ma->member].push_back(addr);
      }
    } else if (auto ca = dynamic_cast<LiteralAddressValue*>(value)) {
      if (ca->literal) {
        // after the literal is laid down, update the address
        addLiteral(ca->literal.get(), addr);
      } // else: nullptr
    } else {
      MCHECK(false, "expect MemberAddressValue or LiteralAddressValue");
    }

    return addr;
  } else {
    return genZero(m->type->getTypeSize(), m->type->getTypeAlign());
  }
}

Address Linker::genReference(SVariableMember m) {
  CHECK(m->initializer);
  auto& initializer = *m->initializer;
  CHECK(!initializer.isDefault() && initializer.expr->isConstant());

  auto literal = initializer.expr->toConstant();
  auto addr = genZero(m->type->getTypeSize(), m->type->getTypeAlign());

  auto value = literal->value.get();
  if (auto ma = dynamic_cast<MemberAddressValue*>(value)) {
    vector<char> bytes;
    if (getAddress(ma->member, bytes)) {
      update(addr, bytes);
    } else {
      addressToMember_[ma->member].push_back(addr);
    }
  } else if (auto ca = dynamic_cast<LiteralAddressValue*>(value)) {
    CHECK(ca->literal);
    addLiteral(ca->literal.get(), addr);
  } else {
    auto ta = dynamic_cast<TemporaryAddressValue*>(value);
    CHECK(ta);
    addTemporary(ta->member, addr);
  }

  return addr;
}

Address Linker::genVariable(SVariableMember m) {
  auto type = m->type;
  if (type->isFundalmental()) {
    return genFundalmental(m);
  } else if (type->isPointer()) {
    return genPointer(m);
  } else if (type->isReference()) {
    return genReference(m);
  } else if (type->isArray()) {
    return genArray(m);
  } else {
    CHECK(false);
    return Address{};
  }
}

void Linker::generateImage() {
  genHeader();

  for (auto& u : units_) {
    auto& ms = u->getVariablesFunctions();
    for (auto& mb : ms) {
      auto m = getUnique(mb);
      if (memberAddress_.find(m) != memberAddress_.end()) {
        // this member has already been laid down
        continue;
      }

      Address addr;
      if (m->isFunction()) {
        addr = genFunction();
      } else {
        addr = genVariable(m->toVariable());
      }

      memberAddress_.insert(make_pair(m, addr));
      // now update any addresses which need to point to this member
      for (auto target : addressToMember_[m]) {
        update(target, getAddress(addr.first));
      }
    }
  }

  for (auto t : temporary_) {
    auto addr = genVariable(t);
    for (auto& target : addressToTemporary_[t]) {
      update(target, getAddress(addr.first));
    }
  }

  for (auto literal : literals_) {
    auto bytes = literal->toBytes();
    auto addr = genEntry(bytes.data(), 
                         bytes.size(), 
                         literal->type->getTypeAlign());
    for (auto& target : addressToLiteral_[literal]) {
      update(target, getAddress(addr.first));
    }
  }
}

auto Linker::process() -> Image {
  checkOdr();
  generateImage();
  return image_;
}

}
