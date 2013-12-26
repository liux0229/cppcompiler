#include "Namespace.h"

namespace compiler {

using namespace std;

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
  } else if (kind != MemberKind::NotDeclared) {
    Throw("{} redeclared to be a namespace; was {}", kind);
  }

  auto ns = make_unique<Namespace>(name, unnamed, isInline, this);
  auto ret = namespaces_.insert(make_pair(name, move(ns)));
  CHECK(ret.second);
  declarationOrder_.ns.push_back(name);

  return ret.first->second.get();
}

void Namespace::addMember(const string& name, SType type) {
  // TODO: ODR
  if (type->isFunction()) {
    functions_.insert(make_pair(name, 
                                static_pointer_cast<FunctionType>(type)));
    declarationOrder_.func.push_back(name);
  } else {
    variables_.insert(make_pair(name, type));
    declarationOrder_.var.push_back(name);
  }
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

}
