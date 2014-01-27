#include "Member.h"
#include "Namespace.h"

namespace compiler {

using namespace std;

string Member::getQualifiedName() const {
  // for temporary variable members, owner can be nullptr
  // we need to further refine this
  return owner ? 
           format("{}::{}", owner->getName(), name) :
           name;
}

ostream& operator<<(ostream& out, MemberKind kind) {
  switch (kind) {
    case MemberKind::Namespace:
      out << "namespace";
      break;
    case MemberKind::Function:
      out << "function";
      break;
    case MemberKind::Variable:
      out << "variable";
      break;
    case MemberKind::Typedef:
      out << "typedef";
      break;
  }
  return out;
}

ostream& operator<<(ostream& out, const Member& m) {
  m.output(out);
  return out;
}

ostream& operator<<(ostream& out, SMember m) {
  m->output(out);
  return out;
}

void Member::output(std::ostream& out) const {
  if (owner && !owner->isGlobal()) {
    out << format("{}::", owner->getName());
  }
  out << format("{} => ", name);
}

void TypedefMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {}]", getKind(), *type);
}

void VariableMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {} {} {}]", 
                getKind(), 
                linkage, 
                storage,
                *type);
}

void FunctionMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("[{} {} {}{}]", 
                getKind(), 
                linkage, 
                *type,
                isInline ? " inline" : "");
}

bool NamespaceMember::isNamespaceAlias(Namespace* enclosing, 
                                       const string& memberName) const {
  // TODO: improve this check
  return !ownedBy(enclosing) || memberName != name;
}

void NamespaceMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("{}", getKind());
}

}
