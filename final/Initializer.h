#pragma once
#include "Expression.h"

namespace compiler {

struct Initializer {
  enum Kind {
    Default,
    // Value,
    Copy,
    // Direct
  };

  Initializer(Kind k, SExpression e) : kind(k), expr(e) { 
    if (k == Copy && !expr) {
      Throw("Copy initializer must have an initializer-expression");
    }
  }

  bool isDefault() const {
    return kind == Default;
  }

  Kind kind;
  SExpression expr; 
};
MakeUnique(Initializer);

inline std::ostream& 
operator<<(std::ostream& out, const Initializer& initializer) {
  if (initializer.isDefault()) {
    out << "default-initializer";
  } else {
    out << "copy-initializer: " << *initializer.expr
        << " is-constant: " << initializer.expr->isConstant();
  }
  return out;
}

}
