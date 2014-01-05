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

  Initializer(Kind k, UExpression&& e) : kind(k), expr(move(e)) { 
    if (k == Copy && !e) {
      Throw("Copy initializer must have an initializer-expression");
    }
  }
  
  Kind kind;
  UExpression expr; 
};

}
