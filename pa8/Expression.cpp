#include "Expression.h"
#include "Member.h"

namespace compiler {

using namespace std;

SExpression Expression::assignableTo(SType target) const {
  auto expr = shared_from_this();
  if (target->isFundalmental() && getType()->isFundalmental()) {
    if (!isPRValue()) {
      expr = make_shared<LValueToRValueConversion>(expr);
    }
    if (expr->getType()->equalsIgnoreCv(*target)) {
      return expr;
    } else {
      if (FundalmentalTypeConversion::allowed(
            expr->getType()->toFundalmental(), 
            target->toFundalmental())) {
        return make_shared<FundalmentalTypeConversion>(
                 expr,
                 target->toFundalmental());
      }
    }
  } else if (target->isArray()) {
    if (getType()->isArray()) {
      auto cur = getType()->toArray();
      cout << format("target: {} from: {}", *target, *cur) << endl;
      if (*cur == *target) {
        return expr;
      } else {
        auto ta = target->toArray();
        if (cur->addSizeTo(*ta)) {
          return expr;
        } else {
          if (ta->getArraySize() > cur->getArraySize()) {
            return expr;
          }
        }
      }
    }
    
    return nullptr;
  } else if (target->isPointer()) {
    if (getType()->isArray()) {
      expr = make_shared<ArrayToPointerConversion>(expr);
    } else if (getType()->isFunction()) {
      expr = make_shared<FunctionToPointerConversion>(expr);
    }

    if (!expr->isPRValue()) {
      expr = make_shared<LValueToRValueConversion>(expr);
    }

    // TODO: fix const int* p = nullptr;
    if (PointerConversion::allowed(expr)) {
      expr = make_shared<PointerConversion>(expr, target->toPointer());
    }

    if (!expr->getType()->isPointer()) {
      return nullptr;
    }

    if (expr->getType()->equalsIgnoreCv(*target)) {
      return expr;
    } else if (QualificationConversion::allowed(
                 expr->getType()->toPointer(),
                 target->toPointer())) {
      return make_shared<QualificationConversion>(expr, target->toPointer());
    } else {
      return nullptr;
    }
  }

  return nullptr;
}

IdExpression::IdExpression(SMember e) : entity(e) {
}

SType IdExpression::getType() const {
  if (entity->isFunction()) {
    return entity->toFunction()->type;
  } else {
    return entity->toVariable()->type;
  }
}

bool IdExpression::isConstant() const {
  if (entity->isFunction()) {
    return false;
  } else {
    // TODO: consider making this a field on VariableMember
    if (!entity->isDefined) {
      return false;
    }
    auto var = entity->toVariable();
    if (!var->type->isConst()) {
      return false;
    }
    auto& initializer = var->initializer;
    CHECK(initializer && !initializer->isDefault());
    return initializer->expr->isConstant();
  }
}

SLiteralExpression IdExpression::toConstant() const {
  CHECK(isConstant());
  CHECK(entity->isVariable());
  auto var = entity->toVariable();
  return var->initializer->expr->toConstant();
}

void IdExpression::output(ostream& out) const {
  out << "IdExpression(" << *entity << ")";
}

bool FundalmentalTypeConversion::allowed(SFundalmentalType from, 
                                         SFundalmentalType to) {
  if (from->isVoid() || to->isVoid()) {
    return false;
  }
  if (from->getType() == FT_NULLPTR_T || to->getType() == FT_NULLPTR_T) {
    return false;
  }
  return true;
}

SLiteralExpression FundalmentalTypeConversion::toConstant() const {
  auto c = from->toConstant();
  auto value = c->value->to(type->toFundalmental()->getType());
  return make_shared<LiteralExpression>(move(value)); 
}

SLiteralExpression ArrayToPointerConversion::toConstant() const {
  UConstantValue value;
  if (from->isConstant()) {
    value = make_unique<LiteralAddressValue>(
              getType(),
              from->toConstant()->value->clone());
  } else {
    auto idExpr = dynamic_cast<const IdExpression*>(from.get());
    CHECK(idExpr);
    value = make_unique<MemberAddressValue>(getType(), idExpr->entity);
  }
  return make_shared<LiteralExpression>(move(value));
}

SLiteralExpression FunctionToPointerConversion::toConstant() const {
  CHECK(!from->isConstant());
  auto idExpr = dynamic_cast<const IdExpression*>(from.get());
  CHECK(idExpr);
  auto value = make_unique<MemberAddressValue>(getType(), idExpr->entity);
  return make_shared<LiteralExpression>(move(value));
}

bool PointerConversion::allowed(SExpression from) {
  if (from->isConstant()) {
    return false;
  }
  auto literal = from->toConstant();
  auto type = literal->getType();
  if (!type->isFundalmental()) {
    return false;
  }

  auto ft = type->toFundalmental();
  if (ft->getType() == FT_NULLPTR_T) {
    return true;
  } else {
    return ft->isInteger() && 
           static_cast<FundalmentalValueBase*>(literal->value.get())
             ->isZero();
  }
}

bool QualificationConversion::allowed(SPointerType from, SPointerType to) {
  // TODO: make this more efficient
  vector<CvQualifier> cvFrom;
  vector<CvQualifier> cvTo;

  do {
    auto fd = from->getDepended();
    auto td = to->getDepended();
    cvFrom.push_back(fd->getCvQualifier());
    cvTo.push_back(td->getCvQualifier());
    if (fd->equalsIgnoreCv(*td)) {
      break;
    } else {
      if (!fd->isPointer() || !td->isPointer()) {
        return false;
      } else {
        from = fd->toPointer();
        to = td->toPointer();
      }
    }
  } while (true);

  // note: this implementation will return true if the pointer types are the
  // same, which is fine.
  // TODO: probably a QualificationConversion implementation which reduces to
  // empty in this situation
  bool requireConst = false;
  for (int i = cvFrom.size() - 1; i >= 0; i--) {
    if (!(cvTo[i] >= cvFrom[i])) {
      return false;
    }
    if (requireConst && !cvTo[i].isConst()) {
      return false;
    }
    if (cvTo[i] != cvFrom[i]) {
      requireConst = true;
    }
  }

  return true;
}

}
