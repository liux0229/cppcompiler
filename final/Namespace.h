#pragma once

#include "common.h"
#include "Type.h"
#include "DeclSpecifiers.h"
#include "Member.h"

namespace compiler {

class TranslationUnit;

class Namespace;
MakeUnique(Namespace);
class Namespace {
 public:
  using MemberSet = std::set<SMember>;

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
                   const DeclSpecifiers& declSpecifiers,
                   UInitializer initializer);
  void addTypedef(const std::string& name, SType type);
  void addUsingDirective(Namespace* ns);
  void addUsingDeclaration(const MemberSet& members);
  void addNamespaceAlias(const std::string& name, SNamespaceMember ns);

  std::string getName() const;
  bool isInline() const { return inline_; }
  bool isGlobal() const { return !parent_; }
  bool enclosedBy(const Namespace* other) const;
  Linkage getLinkage() const { return linkage_; }
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

  void checkInitializer(const std::string& name, 
                        SType& type, 
                        UInitializer& initializer,
                        bool isConstExpr);

  std::string name_;
  bool unnamed_;
  bool inline_;
  Linkage linkage_;
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

}
