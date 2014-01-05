#include "Member.h"
#include "Namespace.h"

namespace compiler {

using namespace std;

string Member::getQualifiedName() const {
  return format("{}::{}", owner->getName(), name);
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

void Member::output(std::ostream& out) const {
  if (!owner->isGlobal()) {
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
  out << format("[{} {} {}]", getKind(), linkage, *type);
}

void NamespaceMember::output(std::ostream& out) const {
  Member::output(out);
  out << format("{}", getKind());
}

}
