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

Namespace::MemberKind Namespace::lookupMember(const string& name) const {
  if (namespaces_.find(name) != namespaces_.end()) {
    return MemberKind::Namespace;
  } else if (functions_.find(name) != functions_.end()) {
    return MemberKind::Function;
  } else if (variables_.find(name) != variables_.end()) {
    return MemberKind::Variable;
  } else if (typedefs_.find(name) != typedefs_.end()) {
    return MemberKind::Typedef;
  } else {
    return MemberKind::NotDeclared;
  }
}

Namespace* Namespace::addNamespace(string name,
                                   bool unnamed, 
                                   bool isInline) {
  if (unnamed) {
    name = getUniqueName();
  }

  auto kind = lookupMember(name);
  if (kind == MemberKind::Namespace) {
    return namespaces_[name].get();
  } else {
    checkUndeclared(kind, name, "namespace");
  }

  auto ns = make_unique<Namespace>(name, unnamed, isInline, this);
  auto ret = namespaces_.insert(make_pair(name, move(ns)));
  CHECK(ret.second);
  declarationOrder_.ns.push_back(name);

  return ret.first->second.get();
}

void Namespace::addFunction(const string& name, 
                            SType type, 
                            MemberKind kind) {
  if (kind == MemberKind::Function) {
    // TODO: handle overloads
    return;
  } else {
    checkUndeclared(kind, name, *type);
  }

  functions_.insert(make_pair(name, 
                              static_pointer_cast<FunctionType>(type)));
  declarationOrder_.func.push_back(name);
}

void Namespace::addVariable(const string& name, SType type, MemberKind kind) {
  if (kind == MemberKind::Variable) {
    auto& exist = variables_[name];
    if (*exist != *type) {
      Throw("{} redeclared to be {}; was {}", name, *type, *exist);
    }
    return;
  } else {
    checkUndeclared(kind, name, *type);
  }

  variables_.insert(make_pair(name, type));
  declarationOrder_.var.push_back(name);
}

void Namespace::addMember(const string& name, SType type) {
  auto kind = lookupMember(name);
  if (type->isFunction()) {
    addFunction(name, type, kind);
  } else {
    addVariable(name, type, kind);
  }
}

void Namespace::addTypedef(const string& name, SType type) {
  auto kind = lookupMember(name);
  if (kind == MemberKind::Typedef) {
    if (*typedefs_[name] != *type) {
      Throw("{} typedef'd to be a different type {}; was {}", 
            name, 
            *type,
            *typedefs_[name]);
    }
    return;
  } else {
    checkUndeclared(kind, name, *type);
  }
  typedefs_.insert(make_pair(name, type));
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
    out << "variable " << name << " " << *variables_.at(name) << endl;
  }
  for (auto& name : declarationOrder_.func) {
    out << "function " << name << " " << *functions_.at(name) << endl;
  }
  for (auto& name : declarationOrder_.ns) {
    out << *namespaces_.at(name);
  }

  out << "end namespace" << endl;
}

Namespace::VMember
Namespace::lookup(const string& name, MemberKind kind) const {
  auto it = typedefs_.find(name);
  if (it != typedefs_.end()) {
    return {Member{it->second}};
  }
  if (parent_) {
    return parent_->lookup(name, kind);
  } else {
    return {};
  }
}

}
