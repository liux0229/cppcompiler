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
        return make_shared<FundalmentalTypeConversion>(expr, target);
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

    if (!expr->getType()->isPointer()) {
      return nullptr;
    }

    if (!expr->isPRValue()) {
      expr = make_shared<LValueToRValueConversion>(expr);
    }

    if (expr->getType()->equalsIgnoreCv(*target)) {
      return expr;
    } else if (QualificationConversion::allowed(
                 expr->getType()->toPointer(),
                 target->toPointer())) {
      return make_shared<QualificationConversion>(expr, target);
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
    return true;
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
  if (entity->isFunction()) {
    Throw("Not implemented");
    return nullptr;
  } else {
    auto var = entity->toVariable();
    return var->initializer->expr->toConstant();
  }
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
