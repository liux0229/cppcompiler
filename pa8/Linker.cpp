#include "Linker.h"

namespace compiler {

using namespace std;

namespace {

using SMember = Namespace::SMember;
using SVariableMember = Namespace::SVariableMember;
using SFunctionMember = Namespace::SFunctionMember;

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

}

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
    }
  }
  for (auto& kv : members_) {
    if (!kv.second->isDefined) {
      Throw("{} not defined", *kv.second);
    }
  }
}

void Linker::process() {
  checkOdr();
}

}
