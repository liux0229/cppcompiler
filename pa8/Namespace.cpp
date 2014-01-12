#include "Namespace.h"
#include "TranslationUnit.h"

namespace compiler {

using namespace std;

Namespace::Namespace(const string& name, 
                     bool unnamed, 
                     bool isInline,
                     const Namespace* parent,
                     TranslationUnit* unit)
  : name_(name),
    unnamed_(unnamed),
    inline_(isInline),
    parent_(parent),
    unit_(unit) {
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

SMember Namespace::lookupMember(const string& name) const {
  MemberSet members;
  lookupMember(name, members);
  if (members.size() > 1) {
    bool allFunc = all_of(members.begin(), 
                          members.end(), 
                          [](SMember m) { return m->isFunction(); });
    MCHECK(allFunc, 
           format("lookupMember({}) got more than 1 matching members, "
                  "not all of which are functions: {}",
                  name,
                  members));
  }
  if (members.empty()) {
    return nullptr;
  } else {
    return *members.begin();
  }
}

void Namespace::lookupMember(const string& name, MemberSet& members) const {
  auto p = members_.equal_range(name);
  for (auto it = p.first; it != p.second; ++it) {
    members.insert(it->second);
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
      reportRedeclaration(name, "namespace", *member);
    }
  }

  namespaces_.push_back(
    make_unique<Namespace>(name, unnamed, isInline, this, unit_));

  auto ns = namespaces_.back().get();
  auto m = make_shared<NamespaceMember>(this, name, ns);
  members_.insert(make_pair(name, m));
  declarationOrder_.ns.push_back(m);

  if (unnamed || isInline) {
    addUsingDirective(ns);
  }

  return ns;
}

void Namespace::addFunction(const string& name, 
                            SFunctionType type,
                            bool requireDeclaration,
                            bool isDef,
                            const DeclSpecifiers& declSpecifiers) {
  bool internalLinkage = 
         declSpecifiers.getStorageClass() & StorageClass::Static;

  MemberSet members;
  lookupMember(name, members);

  auto it = find_if(members.begin(), 
                    members.end(), 
                    [](SMember m) { return !m->isFunction(); });
  if (it != members.end()) {
    reportRedeclaration(name, *type, **it);
  }

  for (auto& m : members) {
    auto& func = static_cast<FunctionMember&>(*m);
    if (*type == *func.type) {
      if (!func.ownedBy(this)) {
        Throw("using declaration {} redeclared to be {}: {}",
              name,
              type,
              func);
      }

      // a redeclaration

      if (internalLinkage && func.linkage != Linkage::Internal) {
        Throw("Linkage mismatch for {}; previous: {} current: {}",
              func,
              func.linkage,
              Linkage::Internal);
      }

      if (isDef) {
        if (m->isDefined) {
          Throw("Multiple definitions of {}", *m);
        }
        m->isDefined = true;
      }

      // redeclaration allowed
      return;
    }

    if (type->sameParameterAndQualifier(*func.type)) {
      Throw("cannot overload function {}: {} vs {}", 
            name, 
            *func.type, 
            *type);
    }
  }

  if (requireDeclaration) {
    Throw("{}: {} must be declared in {} first", name, *type, getName());
  }
      
  auto m = make_shared<FunctionMember>(
             this, 
             name, 
             type, 
             internalLinkage ? Linkage::Internal : Linkage::External,
             isDef);
  members_.insert(make_pair(name, m));
  declarationOrder_.func.push_back(m);
  unit_->addMember(m);
}

// Check whether initializer is valid for type
// Add necessary conversions and potentially mutate type
void Namespace::checkInitializer(
                  const string& name, 
                  SType& type, 
                  UInitializer& initializer,
                  bool isConstExpr) {
  if (!initializer) {
    if (type->isConst() || type->isReference()) {
      Throw("{} ({}) cannot be default initialized", name, *type);
    }

    initializer = make_unique<Initializer>(Initializer::Default, nullptr);
    return;
  }

  auto& expr = initializer->expr;
  cout << format("check(BF): {} {} {}", name, *type, *initializer) << endl;

  if (auto r = expr->assignableTo(type)) {
    expr = r;
  } else {
    Throw("cannot initialize {} ({}) with {}", name, *type, *expr);
  }
  if (expr->isConstant()) {
    expr = expr->toConstant();
  } else {
    if (isConstExpr) {
      Throw("constexpr {} initializer is not constant: {}", name, *expr);
    }
  }

  if (type->isArray() && 
      expr->getType()->isArray()) {
    auto ea = expr->getType()->toArray();
    if (ea->addSizeTo(*type->toArray())) {
      type = ea; 
    }
  }

  cout << format("check(AF): {} {} {}", name, *type, *initializer) << endl;
}

void Namespace::addVariable(const string& name, 
                            SType type, 
                            bool requireDeclaration,
                            const DeclSpecifiers& declSpecifiers,
                            UInitializer initializer) {
  auto storageClass = declSpecifiers.getStorageClass();
  // variable with extern specifier and without an initializer is not 
  // a definition
  bool isDef = !((storageClass & StorageClass::Extern) && 
                 !initializer);

  if (isDef) {
    checkInitializer(name, type, initializer, declSpecifiers.isConstExpr());
  }

  bool internalLinkage = 
         (storageClass & StorageClass::Static) ||
         (type->isConst() && !(storageClass & StorageClass::Extern));
  bool threadLocalStorage = storageClass & StorageClass::ThreadLocal;

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
          // type mismatch with an existing variable
          // disallowed unless it is adding size to array
          error = true;
          if (exist->type->isArray() && type->isArray()) {
            auto& ea = static_cast<ArrayType&>(*exist->type);
            auto& ta = static_cast<ArrayType&>(*type);
            if (ta.addSizeTo(ea)) {
              // modify type of the existing variable
              exist->type = type;
              error = false;
            }
          }
        }

        if (!error) {
          if (internalLinkage && exist->linkage != Linkage::Internal) {
            Throw("Linkage mismatch for {}; previous: {} current: {}",
                  *exist,
                  exist->linkage,
                  Linkage::Internal);
          }
          if (threadLocalStorage && exist->storage !=
              StorageDuration::ThreadLocal) {
            Throw("Storage duration mismatch for {}; previous {} current: {}",
                  *exist,
                  exist->storage,
                  StorageDuration::ThreadLocal);
          }
        }
      }
    } else {
      error = true;
    }
    if (error) {
      reportRedeclaration(name, *type, *member);
    }

    // a redeclaration
    if (isDef) {
      if (member->isDefined) {
        Throw("Multiple definitions of {}", *member);
      }
      member->toVariable()->initializer = move(initializer);
      member->isDefined = true;
    }

    return;
  }

  if (requireDeclaration) {
    Throw("{} must be declared in {} first", name, getName());
  }

  auto m = make_shared<VariableMember>(
             this, 
             name, 
             type, 
             internalLinkage ? 
               Linkage::Internal : 
               // same as the owning namespace (and we haven't made
               // unnamed namespaces explicitly internal)
               Linkage::External,
             threadLocalStorage ? 
               StorageDuration::ThreadLocal :
               StorageDuration::Static,
             isDef,
             move(initializer));
  members_.insert(make_pair(name, m));
  declarationOrder_.var.push_back(m);
  unit_->addMember(m);
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
      reportRedeclaration(name, *type, *member);
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
    // note that because of the "same member" check below, this should be
    // redundant; keep it here for now for clarity
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
      bool ignore = true;

      if (exist->isFunction()) {
        if (!m->isFunction()) {
          error = true;
        } else {
          MemberSet members;
          lookupMember(m->name, members);

          // cout << "lookupMember returns: " << members << endl;

          // if this using declaration refers to a member we currently don't
          // have and its type is compatible with existing ones,
          // then we should add it
          ignore = false;
          for (auto& e : members) {
            // see the comment about the same member below
            if (e != m) {
              auto fe = e->toFunction();
              auto fm = m->toFunction();
              if (fe->type->sameParameterAndQualifier(*fm->type)) {
                error = true;
                // so we get proper error reporting below
                exist = e;
                break;
              }
            } else {
              // we currently have this member
              ignore = true;
              break;
            }
          }
        }
      } else {
        // if they don't refer to the same member
        // e.g. this could happen when two namespaces have using declarations
        // for the same member from a 3rd namespace, and a 4th namespace
        // has using declarations refering to the using declaration member
        // in the first two namespaces
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
      }
      
      if (error) {
        Throw("{} redeclared by a using-declaration to be {}; was {}",
              exist->name, 
              *m,
              *exist);
      }

      if (ignore) {
        return;
      }
    }

    // cout << format("using declaration: added: {} {}", m->name, m) << endl;
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

STypedefMember
Namespace::lookupTypedef(const string& name, bool qualified) const {
  auto members = qualified ? qualifiedLookup(name) : unqualifiedLookup(name);
  CHECK(members.size() <= 1);
  if (members.empty() || !(*members.begin())->isTypedef()) {
    return nullptr;
  }
  return (*members.begin())->toTypedef();
}

SNamespaceMember 
Namespace::lookupNamespace(const string& name, bool qualified) const 
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

bool Namespace::enclosedBy(const Namespace* other) const {
  return getLCA(this, other) == other;
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

void Namespace::output(ostream& out) const {
  if (!unnamed_) {
    out << "start namespace " << name_ << endl;
  } else {
    out << "start unnamed namespace" << endl;
  }
  if (inline_) {
    out << "inline namespace" << endl;
  }

  for (auto& m : declarationOrder_.var) {
    out << "variable " << m->name << " " << *m->type << endl;
  }
  for (auto& m : declarationOrder_.func) {
    out << "function " << m->name << " " << *m->type << endl;
  }
  for (auto& m : declarationOrder_.ns) {
    out << *m->ns;
  }

  out << "end namespace" << endl;
}

}
