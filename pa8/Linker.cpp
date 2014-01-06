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

      cout << "Linking: " << *m;
      if (m->isVariable()) {
        cout << " " << *m->toVariable()->initializer;
      }
      cout << endl;
    }
  }
  for (auto& kv : members_) {
    if (!kv.second->isDefined) {
      Throw("{} not defined", *kv.second);
    }
  }
}

void Linker::genEntry(const char* data, size_t n, size_t alignment) {
  auto start = ((image_.size() + alignment - 1) / alignment) * alignment;
  while (image_.size() < start) {
    image_.push_back(0);
  }
  image_.insert(image_.end(), data, data + n);
}


void Linker::genHeader() {
  gen("PA8");
}

void Linker::genFunction() {
  gen("fun");
}

void Linker::genZero(size_t n, size_t alignment) {
  vector<char> v(n);
  genEntry(v.data(), n, alignment); 
}

// TODO: consider making image generation part of Type so we can use
// virtual dispatch
void Linker::genFundalmental(SVariableMember m) {
  CHECK(m->initializer);
  auto& initializer = *m->initializer;
  if (!initializer.isDefault() && initializer.expr->isConstant()) {
    auto literal = initializer.expr->toConstant();
    auto bytes = literal->value->toBytes();
    genEntry(bytes.data(), bytes.size(), m->type->getAlign());
  } else {
    genZero(m->type->getSize(), m->type->getAlign());
  }
}

void Linker::genArray(SVariableMember m) {
  genZero(m->type->getSize(), m->type->getAlign());
}

void Linker::genPointer(SVariableMember m) {
  genZero(m->type->getSize(), m->type->getAlign());
}

void Linker::genReference(SVariableMember m) {
  genZero(m->type->getSize(), m->type->getAlign());
}

void Linker::generateImage() {
  genHeader();

  for (auto& u : units_) {
    auto& ms = u->getVariablesFunctions();
    for (auto& m : ms) {
      if (m->isFunction()) {
        genFunction();
      } else {
        auto vm = m->toVariable();
        auto type = vm->type;
        if (type->isFundalmental()) {
          genFundalmental(vm);
        } else if (type->isPointer()) {
          genPointer(vm);
        } else if (type->isReference()) {
          genReference(vm);
        } else if (type->isArray()) {
          genArray(vm);
        } else {
          CHECK(false);
        }
      }
    }
  }
}

auto Linker::process() -> Image {
  checkOdr();
  generateImage();
  return image_;
}

}
