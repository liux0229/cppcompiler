#pragma once

#include "common.h"
#include "Type.h"

namespace compiler {

class Namespace;
MakeUnique(Namespace);
class Namespace {
 public:
  Namespace(const std::string& name, 
            bool unamed, 
            bool isInline, 
            const Namespace* parent);

  const Namespace* parentNamespace() const { return parent_; }
  Namespace* addNamespace(std::string name, bool unnamed, bool isInline);
  void addMember(const std::string& name, SType type);

  void output(std::ostream& out) const;

 private:
  enum class MemberKind {
    NotDeclared,
    Namespace,
    Variable,
    Function
  };
  friend inline std::ostream& operator<<(std::ostream& out, 
                                         Namespace::MemberKind kind) {
    out << static_cast<int>(kind);
    return out;
  }

  // TODO: use different names for different TUs
  static std::string getUniqueName() { return "<unnamed>"; }
  MemberKind lookupMember(const std::string &name) const;

  std::string name_;
  bool unnamed_;
  bool inline_;
  const Namespace* parent_;

  // members
  std::map<std::string, SType> variables_;
  std::map<std::string, SFunctionType> functions_;
  std::map<std::string, UNamespace> namespaces_;

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
