#pragma once

#include "Type.h"
#include "Namespace.h"

namespace compiler {

struct DerivedTypeList {
  SType root;
  SType tail;
  void append(SType next) {
    if (!root) {
      CHECK(!tail);
      root = next;
      tail = next;
    } else {
      tail->setDepended(next);
      tail = next;
    }
  }
};

struct Id {
  Id(const std::string& id = "", Namespace* n = nullptr) 
    : unqualified(id), ns(n) { 
  }
  bool isQualified() const { return ns; }
  std::string getName() const {
    if (ns) {
      return ns->getName() + "::" + unqualified;
    } else {
      return unqualified;
    }
  }

  const std::string unqualified;
  Namespace* ns;
};
MakeUnique(Id);

class Declarator {
 public:
  Declarator() {}
  explicit Declarator(const Id& id) 
             : id_(id) { 
  }

  Id getId() const { return id_; }
  SType getType() const { return typeList_.root; }
  void appendType(SType type) { typeList_.append(type); }

 private:
  Id id_; 
  DerivedTypeList typeList_;
};
MakeUnique(Declarator);

struct PtrOperator;
MakeUnique(PtrOperator);
struct PtrOperator {
  enum Type {
    LValueRef,
    RValueRef,
    Pointer 
  };

  PtrOperator(Type t, CvQualifier cv = CvQualifier{})
    : type(t), cvQualifier(cv) {
    CHECK(t == Pointer || cv == CvQualifier{});
  }

  Type type;
  CvQualifier cvQualifier;
};

}
