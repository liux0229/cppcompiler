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

ostream& operator<<(ostream& out, const Namespace::Member& m) {
  m.output(out);
  return out;
}

ostream& operator<<(ostream& out, Namespace::SMember m) {
  m->output(out);
  return out;
}

void Namespace::Member::output(std::ostream& out) const {
  if (!owner->isGlobal()) {
    out << format("{}::", owner->getName());
  }
  out << format("{} => ", name);
}

void Namespace::TypedefMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {}]", getKind(), *type);
}

void Namespace::VariableMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {}]", getKind(), *type);
}

void Namespace::FunctionMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {}]", getKind(), *type);
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
  } else if (parent_->isGlobal()) {
    return name_;
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
      if (!exist->ownedBy(this)) {
        // a namespace alias 
        Throw("namespace alias name redeclared to be namespace: {}", name);
      }
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

  if (unnamed || isInline) {
    addUsingDirective(ns);
  }

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
    bool error = false;
    if (member->isVariable()) {
      if (!member->ownedBy(this)) {
        // there was already a using-declaration of a variable for this name
        error = true;
      } else {
        auto exist = member->toVariable();
        if (*exist->type != *type) {
          if (exist->type->isArray() && type->isArray()) {
            auto& ea = static_cast<ArrayType&>(*exist->type);
            auto& ta = static_cast<ArrayType&>(*type);
            if (ta.addSizeTo(ea)) {
              // modify type of the existing variable
              members_[name]->toVariable()->type = type;
            }
          }
        }
      }
    } else {
      error = true;
    }
    if (error) {
      reportRedeclaration(name, *type, member->getKind());
    }
    return;
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

void Namespace::addUsingDeclaration(const MemberSet& ms) {
  for (auto& m : ms) {
    if (m->owner == this) {
      continue;
    }

    if (m->isNamespace()) {
      auto ns = m->toNamespace()->ns;
      Throw("using-declaration refers to a namespace: {}", ns->getName());
    }

    auto exist = lookupMember(m->name); 
    if (exist) {
      bool error = false;

      // if they don't refer to the same member
      if (exist != m) { 
        if (exist->getKind() != m->getKind()) {
          error = true;
        } else if (m->isTypedef()) {
          auto em = exist->toTypedef();
          auto tm = m->toTypedef();
          if (tm->type != em->type) {
            error = true;
          }
        } else {
          error = true;
        }
      }
      
      if (error) {
        Throw("{} redeclared by a using-declaration to be {}; was {}",
              exist->name, 
              *m,
              *exist);
      }

      // ignore this redeclaration
      return;
    }

    members_.insert(make_pair(m->name, m));
  }
}

void Namespace::addNamespaceAlias(const std::string& name, 
                                  SNamespaceMember ns) {
  auto exist = lookupMember(name);
  if (exist) {
    if (exist != ns) {
      Throw("{} redeclared to be a namespace-alias; was {}", 
            name,
            *exist);
    }
    // duplicate aliases allowed
    return;
  }
  members_.insert(make_pair(name, ns));
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

void Namespace::checkLookupAmbiguity(const string& name,
                                     MemberSet& members) const {
  // cout << format("lookup result [{}]: {}", name, members) << endl;

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
      exist = tm;
    }
    if (ambiguious) {
      Throw("lookup for {} is ambiguious; found: {}", name, members);
    }
    while (members.size() > 1) {
      members.erase(members.begin());
    }
  }
}

Namespace::MemberSet Namespace::unqualifiedLookup(const string& name) const {
  UsingDirectiveMap usingDirectiveMap;
  auto members = unqualifiedLookup(name, usingDirectiveMap);
  checkLookupAmbiguity(name, members);
  return members;
}

Namespace::MemberSet Namespace::qualifiedLookup(const string& name) const {
  set<const Namespace*> visited;
  MemberSet members = qualifiedLookup(name, visited);
  checkLookupAmbiguity(name, members);
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

auto Namespace::lookupNamespace(const string& name, bool qualified) const 
-> SNamespaceMember 
{
  auto members = qualified ? qualifiedLookup(name) : unqualifiedLookup(name);
  if (members.empty() || !(*members.begin())->isNamespace()) {
    return nullptr;
  }
  return (*members.begin())->toNamespace();
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
