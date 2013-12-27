#pragma once

#include "common.h"
#include "Type.h"

namespace compiler {

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
    virtual MemberKind getKind() const = 0;

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
  };
  MakeShared(Member);
  using VMember = std::vector<SMember>;

  struct TypedefMember : Member {
    TypedefMember(SType t) : type(t) { }
    MemberKind getKind() const override { return MemberKind::Typedef; }
    SType type;
  };

  struct VariableMember : Member {
    VariableMember(SType t) : type(t) { }
    MemberKind getKind() const override { return MemberKind::Variable; }
    SType type;
  };

  struct FunctionMember : Member {
    FunctionMember(SType t) : type(t) { }
    MemberKind getKind() const override { return MemberKind::Function; }
    SType type;
  };

  struct NamespaceMember : Member {
    NamespaceMember(Namespace* n) : ns(n) { }
    MemberKind getKind() const override { return MemberKind::Namespace; }
    Namespace* ns;
  };

  Namespace(const std::string& name, 
            bool unamed, 
            bool isInline, 
            const Namespace* parent);

  const Namespace* parentNamespace() const { return parent_; }
  Namespace* addNamespace(std::string name, bool unnamed, bool isInline);
  void addVariableOrFunction(const std::string& name, SType type);
  void addTypedef(const std::string& name, SType type);

  bool isInline() const { return inline_; }
  void output(std::ostream& out) const;
  VMember lookup(const std::string& name) const;
  STypedefMember lookupTypedef(const std::string& name) const;

 private:
  // TODO: use different names for different TUs
  static std::string getUniqueName() { return "<unnamed>"; }

  template<typename T>
  static void reportRedeclaration(const std::string& name, 
                                  const T& entity,
                                  MemberKind kind) {
    Throw("{} redeclared to be a {}; was {}", name, entity, kind);
  }

  SMember lookupMember(const std::string &name) const;
  void addFunction(const std::string& name, SType type);
  void addVariable(const std::string& name, SType type);

  std::string name_;
  bool unnamed_;
  bool inline_;
  const Namespace* parent_;

  std::map<std::string, SMember> members_;
  // manage lifetime of namespaces
  std::vector<UNamespace> namespaces_;

  struct {
    std::vector<std::string> var;
    std::vector<std::string> func;
    std::vector<std::string> ns;
  } declarationOrder_;
};

inline std::ostream& operator<<(std::ostream& out, const Namespace& ns) {
  ns.output(out);
  return out;
}

}
