#include "Namespace.h"

namespace compiler {

using namespace std;

ostream& operator<<(ostream& out, Namespace::MemberKind kind) {
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
    }
  return out;
}

ostream& operator<<(ostream& out, Namespace::SMember m) {
  m->output(out);
  return out;
}

void Namespace::Member::output(std::ostream& out) const {
  out << format("{}::{} => ", owner->getName(), name);
}

void Namespace::TypedefMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {}]", getKind(), type);
}

void Namespace::VariableMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {}]", getKind(), type);
}

void Namespace::FunctionMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {}]", getKind(), type);
}

void Namespace::NamespaceMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("{}", getKind());
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

string Namespace::getName() const {
  if (!parent_) {
    return ""; 
  } else {
    return parent_->getName() + "::" + name_;
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

void Namespace::lookupMember(const string& name, MemberSet& members) const {
  auto member = lookupMember(name);
  if (member) {
    members.insert(member);
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
  members_.insert(make_pair(name,
                            make_shared<NamespaceMember>(this, name, ns)));
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

  members_.insert(make_pair(name, 
                            make_shared<FunctionMember>(this, name, type)));
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

  members_.insert(make_pair(name, 
                            make_shared<VariableMember>(this, name, type)));
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
  members_.insert(make_pair(name, 
                            make_shared<TypedefMember>(this, name, type)));
}

void Namespace::addUsingDirective(Namespace* ns) {
  if (ns == this) {
    return;
  }
  usingDirectives_.insert(ns);
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

Namespace::MemberSet
Namespace::unqualifiedLookup(const string& name, 
                             UsingDirectiveMap& usingDirectiveMap) const {
  auto closure = getUsingDirectiveClosure();
  // for each namespace in the closure, it's as if the members from that
  // namespace appears in the LCA of this and that namespace
  for (auto ns : closure) {
    auto lca = getLCA(this, ns);
    if (lca != ns) {
      usingDirectiveMap[lca].insert(ns);
    }
  }

  MemberSet members;
  lookupMember(name, members);
  // for this namespace, also lookup the members of the namespaces in the 
  // usingDirectiveMap: it's as if those members appear in this namespace
  for (auto ns : usingDirectiveMap[this]) {
    ns->lookupMember(name, members); 
  }

  if (!members.empty()) {
    return members;
  }

  if (parent_) {
    return parent_->unqualifiedLookup(name, usingDirectiveMap);
  } else {
    return {};
  }
}

void Namespace::checkLookupAmbiguity(MemberSet& members) const {
  // in the current feature set, members either contain 
  // 1) 1 of vars, funcs, or ns
  // 2) multiple typedefs which refer to the same type
  if (members.size() > 1) {
    bool ambiguious = false;
    STypedefMember exist;
    for (auto& m : members) {
      if (!m->isTypedef()) {
        ambiguious = true;
        break;
      }
      auto tm = m->toTypedef();
      if (exist && exist->type != tm->type) {
        ambiguious = true;
      }
    }
    if (ambiguious) {
      Throw("lookup is ambiguious; found: {}", members);
    }
    while (members.size() > 1) {
      members.erase(members.begin());
    }
  }
}

Namespace::MemberSet Namespace::unqualifiedLookup(const string& name) const {
  UsingDirectiveMap usingDirectiveMap;
  auto members = unqualifiedLookup(name, usingDirectiveMap);
  checkLookupAmbiguity(members);
  return members;
}

Namespace::MemberSet Namespace::qualifiedLookup(const string& name) const {
  set<const Namespace*> visited;
  MemberSet members = qualifiedLookup(name, visited);
  checkLookupAmbiguity(members);
  return members;
}

Namespace::MemberSet
Namespace::qualifiedLookup(const string& name, NamespaceSet& visited) const {
  if (visited.find(this) != visited.end()) {
    return {};
  }
  visited.insert(this);

  MemberSet members;
  lookupMember(name, members);
  // lookup inline namespaces as well
  auto closure = getInlineNamespaceClosure();
  for (auto ns : closure) {
    ns->lookupMember(name, members);
  }

  if (!members.empty()) {
    return members;
  }

  for (auto ns : usingDirectives_) {
    auto m = ns->qualifiedLookup(name, visited);
    members.insert(m.begin(), m.end());
  }

  return members;
}

Namespace::STypedefMember
Namespace::lookupTypedef(const string& name, bool qualified) const {
  auto members = qualified ? qualifiedLookup(name) : unqualifiedLookup(name);
  CHECK(members.size() <= 1);
  if (members.empty() || !(*members.begin())->isTypedef()) {
    return nullptr;
  }
  return (*members.begin())->toTypedef();
}

Namespace*
Namespace::lookupNamespace(const string& name, bool qualified) const {
  auto members = qualified ? qualifiedLookup(name) : unqualifiedLookup(name);
  if (members.empty() || !(*members.begin())->isNamespace()) {
    return nullptr;
  }
  return (*members.begin())->toNamespace()->ns;
}

auto Namespace::getUsingDirectiveClosure() const -> NamespaceSet {
  NamespaceSet closure;
  closure.insert(this);
  getUsingDirectiveClosure(closure);
  closure.erase(this);
  return closure;
}

void Namespace::getUsingDirectiveClosure(NamespaceSet& closure) const {
  for (auto ns : usingDirectives_) {
    if (closure.find(ns) == closure.end()) {
      closure.insert(ns);
      ns->getUsingDirectiveClosure(closure);
    }
  }
}

auto Namespace::getInlineNamespaceClosure() const -> NamespaceSet {
  NamespaceSet closure;
  getInlineNamespaceClosure(closure);
  return closure;
}

void Namespace::getInlineNamespaceClosure(NamespaceSet& closure) const {
  for (auto& kv : members_) {
    auto& m = kv.second;
    if (m->isNamespace() && m->ownedBy(this)) {
      auto ns = m->toNamespace()->ns;
      if (ns->isInline()) {
        closure.insert(ns);
        ns->getInlineNamespaceClosure(closure);
      }
    }
  }
}

const Namespace* Namespace::getLCA(const Namespace* a, const Namespace* b) {
  if (a == b) {
    return a;
  }

  auto da = a->getDepth();
  auto db = b->getDepth();
  const Namespace** walk;
  int walkDepth;
  if (da > db) {
    walk = &a;
    walkDepth = da - db;
  } else {
    walk = &b;
    walkDepth = db - da;
  }

  while (walkDepth--) {
    *walk = (*walk)->parent_;
  }

  while (a != b) {
    a = a->parent_;
    b = b->parent_;
  }

  return a;
}

size_t Namespace::getDepth() const {
  if (!parent_) {
    return 0;
  } else {
    return 1 + parent_->getDepth();
  }
}

}
