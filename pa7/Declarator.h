#pragma once

#include "Type.h"

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

using Id = std::string;
MakeUnique(Id);

using DeclaratorId = std::string;
MakeUnique(DeclaratorId);

class Declarator {
 public:
  Declarator() : abstract_(true) { }
  explicit Declarator(const DeclaratorId& id) 
             : abstract_(false),
               id_(id) { 
  }

  DeclaratorId getId() const { return id_; }
  SType getType() const { return typeList_.root; }
  void appendType(SType type) { typeList_.append(type); }

 private:
  bool abstract_;
  DeclaratorId id_; 
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