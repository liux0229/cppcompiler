#pragma once

#include "common.h"
#include "Type.h"

namespace compiler {

class Namespace;
MakeUnique(Namespace);
class Namespace {
 public:
  enum class MemberKind {
    NotDeclared = 0x1,
    Namespace = 0x2,
    Variable = 0x4,
    Function = 0x8,
    Typedef = 0x10,
  };

  struct Member {
    virtual MemberKind getKind() const { return MemberKind::NotDeclared; }
    bool isTypedef() const {
      return getKind() == MemberKind::Typedef;
    }
  };
  MakeUnique(Member);
  using VMember = std::vector<UMember>;

  struct TypedefMember : Member {
    TypedefMember(SType t) : type(t) { }
    MemberKind getKind() const override { return MemberKind::Typedef; }
    SType type;
  };
  MakeUnique(TypedefMember);
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
    NamespaceMember(const Namespace* n) : ns(n) { }
    MemberKind getKind() const override { return MemberKind::Namespace; }
    const Namespace* ns = nullptr;
  };

  Namespace(const std::string& name, 
            bool unamed, 
            bool isInline, 
            const Namespace* parent);

  const Namespace* parentNamespace() const { return parent_; }
  Namespace* addNamespace(std::string name, bool unnamed, bool isInline);
  void addMember(const std::string& name, SType type);
  void addTypedef(const std::string& name, SType type);

  void output(std::ostream& out) const;
  VMember lookup(const std::string& name) const;
  UTypedefMember lookupTypedef(const std::string& name) const;

 private:
  // TODO: use different names for different TUs
  static std::string getUniqueName() { return "<unnamed>"; }

  template<typename T>
  static void checkUndeclared(MemberKind kind, 
                              const std::string& name, 
                              const T& entity) {
    if (kind != MemberKind::NotDeclared) {
      Throw("{} redeclared to be a {}; was {}", name, entity, kind);
    }
  }

  UMember lookupMember(const std::string &name) const;
  void addFunction(const std::string& name, SType type, MemberKind kind);
  void addVariable(const std::string& name, SType type, MemberKind kind);

  std::string name_;
  bool unnamed_;
  bool inline_;
  const Namespace* parent_;

  // members
  std::map<std::string, SType> variables_;
  std::map<std::string, SFunctionType> functions_;
  std::map<std::string, UNamespace> namespaces_;
  std::map<std::string, SType> typedefs_;

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
