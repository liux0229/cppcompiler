#pragma once
#include "common.h"
#include "PostProcessingToken.h"

namespace compiler {

struct IdExpression {
  
};

struct Expression {
  bool isConstant;

  // the value if the expression is constant
  // note that it's not ideal that we couple
  // ourselves with PostToken here, as PostToken
  // is a lower level concept. We should refactor
  // such that we have a ConstantValue
  // concept and use it here; however let's defer
  // that for now
  UTokenLiteral value;

};
MakeUnique(Expression);

}
