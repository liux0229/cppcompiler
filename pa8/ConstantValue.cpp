#include "ConstantValue.h"
#include "Expression.h"
#include "Member.h"

namespace compiler {

using namespace std;

bool MemberAddressValue::isConstant() const {
  return IdExpression(member).isConstant();
}

SLiteralExpression MemberAddressValue::toConstant() const {
  return IdExpression(member).toConstant();
}

SLiteralExpression LiteralAddressValue::toConstant() const {
  return make_shared<LiteralExpression>(literal);
}

TemporaryAddressValue::TemporaryAddressValue(
    SType type, 
    SType temporaryType, 
    SExpression initExpr) 
      : AddressValue(type) {
  if (initExpr->isConstant()) {
    initExpr->toConstant();
  }
  member = make_shared<VariableMember>(
             nullptr,
             "<temp>",
             temporaryType,
             Linkage::External,
             StorageDuration::Static,
             true,
             make_unique<Initializer>(Initializer::Copy, initExpr));
}

bool TemporaryAddressValue::isConstant() const {
  return IdExpression(member).isConstant();
}

SLiteralExpression TemporaryAddressValue::toConstant() const {
  return IdExpression(member).toConstant();
}

}
