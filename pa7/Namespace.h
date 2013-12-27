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
    SType type;
  };
  using VMember = std::vector<Member>;

  Namespace(const std::string& name, 
            bool unamed, 
            bool isInline, 
            const Namespace* parent);

  const Namespace* parentNamespace() const { return parent_; }
  Namespace* addNamespace(std::string name, bool unnamed, bool isInline);
  void addMember(const std::string& name, SType type);
  void addTypedef(const std::string& name, SType type);

  void output(std::ostream& out) const;
  VMember lookup(const std::string& name, MemberKind kind) const;

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

  MemberKind lookupMember(const std::string &name) const;
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
