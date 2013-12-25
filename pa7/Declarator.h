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

struct Declarator {
  Declarator(const DeclaratorId& i) : id(i) { }
  DerivedTypeList typeList;
  DeclaratorId id;  
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
