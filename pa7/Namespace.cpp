#include "Namespace.h"

namespace compiler {

using namespace std;

std::ostream& operator<<(std::ostream& out, Namespace::MemberKind kind) {
  switch (kind) {
    case Namespace::MemberKind::Namespace:
      out << "namespace";
      break;
    case Namespace::MemberKind::Function:
      out << "function";
      break;
    case Namespace::MemberKind::Variable:
      out << "variable";
      break;
    case Namespace::MemberKind::Typedef:
      out << "typedef";
      break;
    default:
      out << "<undeclared>";
      break;
    }
  return out;
}

Namespace::Namespace(const string& name, 
                     bool unnamed, 
                     bool isInline,
                     const Namespace* parent)
  : name_(name),
    unnamed_(unnamed),
    inline_(isInline),
    parent_(parent) {
  if (unnamed_) {
    name_ = getUniqueName();
  }
}

Namespace::SMember Namespace::lookupMember(const string& name) const {
  auto it = members_.find(name);
  if (it != members_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

Namespace* Namespace::addNamespace(string name,
                                   bool unnamed, 
                                   bool isInline) {
  if (unnamed) {
    name = getUniqueName();
  }

  auto member = lookupMember(name);
  if (member) {
    if (member->isNamespace()) {
      auto exist = member->toNamespace();
      if (isInline && !exist->ns->isInline()) {
        Throw("extension-namespace-definition is inline "
              "while the original one was not: {}", name);
      }
      return exist->ns;
    } else {
      reportRedeclaration(name, "namespace", member->getKind());
    }
  }

  namespaces_.push_back(
    make_unique<Namespace>(name, unnamed, isInline, this));

  auto ns = namespaces_.back().get();
  members_.insert(make_pair(name, make_shared<NamespaceMember>(ns)));
  declarationOrder_.ns.push_back(name);

  return ns;
}

void Namespace::addFunction(const string& name, 
                            SType type) {
  auto member = lookupMember(name);
  if (member) {
    if (member->isFunction()) {
      // TODO: handle overloads
      return;
    }
    reportRedeclaration(name, *type, member->getKind());
  }

  members_.insert(make_pair(name, make_shared<FunctionMember>(type)));
  declarationOrder_.func.push_back(name);
}

void Namespace::addVariable(const string& name, SType type) {
  auto member = lookupMember(name);
  if (member) {
    if (member->isVariable()) {
      auto exist = member->toVariable();
      if (*exist->type != *type) {
        Throw("{} redeclared to be {}; was {}", name, *type, *exist->type);
      }
      return;
    }
    reportRedeclaration(name, *type, member->getKind());
  }

  members_.insert(make_pair(name, make_shared<VariableMember>(type)));
  declarationOrder_.var.push_back(name);
}

void Namespace::addVariableOrFunction(const string& name, SType type) {
  if (type->isFunction()) {
    addFunction(name, type);
  } else {
    addVariable(name, type);
  }
}

void Namespace::addTypedef(const string& name, SType type) {
  auto member = lookupMember(name);
  if (member) {
    if (member->isTypedef()) {
      auto exist = member->toTypedef();
      if (*exist->type != *type) {
        Throw("{} typedef'd to be a different type {}; was {}", 
              name, 
              *type,
              *exist->type);
      }
    } else {
      reportRedeclaration(name, *type, member->getKind());
    }
  }
  members_.insert(make_pair(name, make_shared<TypedefMember>(type)));
}

void Namespace::output(ostream& out) const {
  if (!unnamed_) {
    out << "start namespace " << name_ << endl;
  } else {
    out << "start unnamed namespace" << endl;
  }
  if (inline_) {
    out << "inline namespace" << endl;
  }

  for (auto& name : declarationOrder_.var) {
    out << "variable " << name << " " 
        << *lookupMember(name)->toVariable()->type << endl;
  }
  for (auto& name : declarationOrder_.func) {
    out << "function " << name << " "
        << *lookupMember(name)->toFunction()->type << endl;
  }
  for (auto& name : declarationOrder_.ns) {
    out << *lookupMember(name)->toNamespace()->ns;
  }

  out << "end namespace" << endl;
}

Namespace::VMember
Namespace::lookup(const string& name) const {
  auto member = lookupMember(name);
  if (!member) {
    if (parent_) {
      return parent_->lookup(name);
    } else {
      return {};
    }
  }
  VMember ret;
  ret.push_back(member);
  return ret;
}

Namespace::STypedefMember
Namespace::lookupTypedef(const string& name) const {
  auto members = lookup(name);
  if (members.empty() || !members[0]->isTypedef()) {
    return nullptr;
  }
  return static_pointer_cast<TypedefMember>(members[0]);
}


}
