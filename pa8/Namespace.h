#pragma once

#include "common.h"
#include "Type.h"
#include "StorageClass.h"
#include "DeclSpecifiers.h"

namespace compiler {

class TranslationUnit;

class Namespace;
MakeUnique(Namespace);
class Namespace {
 public:
  enum class MemberKind {
    Namespace = 0x1,
    Variable = 0x2,
    Function = 0x4,
    Typedef = 0x8,
  };

  class NamespaceMember;
  class VariableMember;
  class FunctionMember;
  class TypedefMember;
  MakeShared(NamespaceMember);
  MakeShared(VariableMember);
  MakeShared(FunctionMember);
  MakeShared(TypedefMember);

  struct Member : std::enable_shared_from_this<Member> {
    Member(Namespace* o, 
           const std::string& n, 
           Linkage link, 
           bool defined) 
      : owner(o), name(n), linkage(link), isDefined(defined) { }

    virtual MemberKind getKind() const = 0;
    virtual void output(std::ostream& out) const = 0;

    bool isVariable() const {
      return getKind() == MemberKind::Variable;
    }
    bool isFunction() const {
      return getKind() == MemberKind::Function;
    }
    bool isTypedef() const {
      return getKind() == MemberKind::Typedef;
    }
    bool isNamespace() const {
      return getKind() == MemberKind::Namespace;
    }

    SVariableMember toVariable() {
      CHECK(isVariable());
      return std::static_pointer_cast<VariableMember>(shared_from_this());
    }
    SFunctionMember toFunction() {
      CHECK(isFunction());
      return std::static_pointer_cast<FunctionMember>(shared_from_this());
    }
    STypedefMember toTypedef() {
      CHECK(isTypedef());
      return std::static_pointer_cast<TypedefMember>(shared_from_this());
    }
    SNamespaceMember toNamespace() {
      CHECK(isNamespace());
      return std::static_pointer_cast<NamespaceMember>(shared_from_this());
    }

    bool ownedBy(const Namespace* n) const {
      return n == owner;
    }

    std::string getQualifiedName() const {
      return format("{}::{}", owner->getName(), name);
    }

    Namespace* owner;
    const std::string name;
    // TODO: revisit this design later (does linkage naturally apply to every
    // kind of member)
    Linkage linkage;
    bool isDefined;
  };
  MakeShared(Member);
  using MemberSet = std::set<SMember>;

  struct TypedefMember : Member {
    TypedefMember(Namespace* owner, const std::string& name, SType t) 
      : Member(owner, 
               name, 
               Linkage::Internal, 
               true), 
        type(t) { }

    MemberKind getKind() const override { return MemberKind::Typedef; }
    void output(std::ostream& out) const override;

    SType type;
  };

  struct VariableMember : Member {
    VariableMember(Namespace* owner, 
                   const std::string& name, 
                   SType t,
                   Linkage link,
                   StorageDuration store,
                   bool isDef) 
      : Member(owner, 
               name, 
               link,
               isDef), 
        type(t),
        storage(store) { 
    }
    MemberKind getKind() const override { return MemberKind::Variable; }
    void output(std::ostream& out) const override;

    SType type;
    StorageDuration storage;
  };

  struct FunctionMember : Member {
    FunctionMember(Namespace* owner, 
                   const std::string& name, 
                   SFunctionType t,
                   Linkage link,
                   bool isDef) 
      : Member(owner, 
               name, 
               link,
               isDef), 
        type(t) { }
    MemberKind getKind() const override { return MemberKind::Function; }
    void output(std::ostream& out) const override;

    SFunctionType type;
  };

  struct NamespaceMember : Member {
    NamespaceMember(Namespace* owner, const std::string& name, Namespace* n) 
      : Member(owner, name, Linkage::External, true), ns(n) { }
    MemberKind getKind() const override { return MemberKind::Namespace; }
    void output(std::ostream& out) const override;

    Namespace* ns;
  };

  Namespace(const std::string& name, 
            bool unamed, 
            bool isInline, 
            const Namespace* parent,
            TranslationUnit* unit);

  const Namespace* parentNamespace() const { return parent_; }
  Namespace* addNamespace(std::string name, bool unnamed, bool isInline);
  void addFunction(const std::string& name, 
                   SFunctionType type, 
                   bool requireDeclaration,
                   bool isDef,
                   const DeclSpecifiers& declSpecifiers);
  void addVariable(const std::string& name, 
                   SType type, 
                   bool requireDeclaration,
                   const DeclSpecifiers& declSpecifiers);
  void addTypedef(const std::string& name, SType type);
  void addUsingDirective(Namespace* ns);
  void addUsingDeclaration(const MemberSet& members);
  void addNamespaceAlias(const std::string& name, SNamespaceMember ns);

  std::string getName() const;
  bool isInline() const { return inline_; }
  bool isGlobal() const { return !parent_; }
  bool enclosedBy(const Namespace* other) const;
  void output(std::ostream& out) const;
  STypedefMember lookupTypedef(const std::string& name, 
                               bool qualified) const;
  SNamespaceMember lookupNamespace(const std::string& name, 
                                   bool qualified) const;
  MemberSet unqualifiedLookup(const std::string& name) const;
  MemberSet qualifiedLookup(const std::string& name) const;

 private:
  using NamespaceSet = std::set<const Namespace*>;

  // TODO: use different names for different TUs
  static std::string getUniqueName() { return "<unnamed>"; }
  static const Namespace* getLCA(const Namespace* a, const Namespace* b);
  size_t getDepth() const;

  template<typename T>
  static void reportRedeclaration(const std::string& name, 
                                  const T& entity,
                                  const Member& exist) {
    Throw("{} redeclared to be a {}; was {}", name, entity, exist);
  }

  SMember lookupMember(const std::string &name) const;
  void lookupMember(const std::string &name, MemberSet& members) const;

  void getUsingDirectiveClosure(NamespaceSet& closure) const;
  NamespaceSet getUsingDirectiveClosure() const;
  void getInlineNamespaceClosure(NamespaceSet& closure) const;
  NamespaceSet getInlineNamespaceClosure() const;

  // Ns A -> [Ns]: namespace A needs to consider [Ns] in its unqualified lookup
  using UsingDirectiveMap = std::map<const Namespace*, NamespaceSet>;
  void checkLookupAmbiguity(const std::string& name, MemberSet& members) const;
  MemberSet unqualifiedLookup(const std::string& name, 
                              UsingDirectiveMap& usingDirectiveMap) const;
  MemberSet qualifiedLookup(const std::string& name, 
                            NamespaceSet& visited) const;

  std::string name_;
  bool unnamed_;
  bool inline_;
  const Namespace* parent_;
  TranslationUnit* unit_;

  std::multimap<std::string, SMember> members_;
  // manage lifetime of namespaces
  std::vector<UNamespace> namespaces_;
  std::set<Namespace*> usingDirectives_;

  struct {
    std::vector<SVariableMember> var;
    std::vector<SFunctionMember> func;
    std::vector<SNamespaceMember> ns;
  } declarationOrder_;
};

inline std::ostream& operator<<(std::ostream& out, const Namespace& ns) {
  ns.output(out);
  return out;
}

std::ostream& operator<<(std::ostream& out, const Namespace::Member& m);

}
