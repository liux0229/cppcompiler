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
  } else if (target->isReference()) {
    auto ref = target->toReference();
    if (auto bind = ReferenceBinding::create(expr, ref)) {
      return bind;
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
    auto var = entity->toVariable();
    if (var->type->isReference()) {
      auto ref = var->type->toReference();
      return ref->getDepended();
    } else {
      return var->type;
    }
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
    if (!var->type->isReference()) {
      if (!var->type->isConst()) {
        return false;
      }
      auto& initializer = var->initializer;
      CHECK(initializer && !initializer->isDefault());
      return initializer->expr->isConstant();
    } else {
      // the constness of reference should reflect the constness of the
      // refered-to

      auto& initializer = var->initializer;
      CHECK(initializer && !initializer->isDefault());
      if (!initializer->expr->isConstant()) {
        return false;
      }
      auto literal = initializer->expr->toConstant();
      auto& address = static_cast<AddressValue&>(*literal->value);
      return address.isConstant();
    }
  }
}

SLiteralExpression IdExpression::toConstant() const {
  CHECK(isConstant());
  CHECK(entity->isVariable());
  auto var = entity->toVariable();
  if (!var->type->isReference()) {
    return var->initializer->expr->toConstant();
  } else {
    auto literal = var->initializer->expr->toConstant();
    auto& address = static_cast<AddressValue&>(*literal->value);
    return address.toConstant();
  }
}

SAddressValue IdExpression::getAddress() const {
  if (entity->isFunction()) {
    return make_shared<MemberAddressValue>(getType(), entity);
  } else {
    CHECK(entity->isVariable());
    auto var = entity->toVariable();
    if (!var->type->isReference()) {
      return make_shared<MemberAddressValue>(getType(), entity);
    } else {
      CHECK(var->isDefined && 
            var->initializer && !var->initializer->isDefault());
      auto& expr = var->initializer->expr;
      CHECK(expr->isConstant());
      SLiteralExpression literal = expr->toConstant();
      auto address = dynamic_pointer_cast<AddressValue>(literal->value);
      CHECK(address);
      return address;
    }
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

SLiteralExpression ArrayToPointerConversion::toConstant() const {
  SConstantValue value;
  if (from->isConstant()) {
    value = make_shared<LiteralAddressValue>(
              getType(),
              from->toConstant()->value);
  } else {
    auto idExpr = dynamic_cast<const IdExpression*>(from.get());
    CHECK(idExpr);
    value = make_shared<MemberAddressValue>(getType(), idExpr->entity);
  }
  return make_shared<LiteralExpression>(value);
}

SLiteralExpression FunctionToPointerConversion::toConstant() const {
  CHECK(!from->isConstant());
  auto idExpr = dynamic_cast<const IdExpression*>(from.get());
  CHECK(idExpr);
  auto value = make_unique<MemberAddressValue>(getType(), idExpr->entity);
  return make_shared<LiteralExpression>(move(value));
}

bool PointerConversion::allowed(SExpression from) {
  cout << *from << endl;
  if (!from->isConstant()) {
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

SReferenceBinding ReferenceBinding::create(SExpression e, 
                                           SReferenceType target) {
  auto fromType = e->getType();
  auto toType = target->getDepended();
  if (target->getKind() == ReferenceType::LValueRef &&
      e->valueCategory() == ValueCategory::LValue) {
    if (toType->isReferenceCompatible(*fromType)) {
      return make_shared<ReferenceBinding>(e, target);
    }
  }

  if (target->getKind() == ReferenceType::RValueRef ||
      (toType->getCvQualifier() == CvQualifier::Const ||
       toType->isArray())) {

    // array prvalue
    // TODO: the treatment of array value's value category might be wrong
    if (e->isConstant() && e->valueCategory() == ValueCategory::PRValue) {
      auto literal = e->toConstant();
      if (dynamic_cast<ArrayValueBase*>(literal->value.get())) {
        if (toType->isReferenceCompatible(*literal->value->type)) {
          return make_shared<ReferenceBinding>(literal, target);
        }
      }
    }

    // bound to temporary
    if (toType->isReferenceRelated(*fromType)) {
      if (target->getKind() == ReferenceType::RValueRef) {
        return nullptr;
      }
      if (!(toType->getCvQualifier() >= fromType->getCvQualifier())) {
        return nullptr;
      }
    }

    if (auto expr = e->assignableTo(toType)) {
      return make_shared<ReferenceBinding>(expr, target);
    }
  }

  return nullptr;
}

bool ReferenceBinding::isConstant(const IdExpression* id) const {
  auto& entity = id->entity;
  if (entity->isFunction()) {
    return true; 
  } else {
    CHECK(entity->isVariable());
    auto var = entity->toVariable();
    if (!var->type->isReference()) {
      // bound to a non-reference variable; can directly take address
      return true;
    } else {
      // bound to a reference; whether the reference is already bound to
      // an address
      if (!var->isDefined) {
        return false;
      }

      CHECK(var->initializer && !var->initializer->isDefault());
      auto& expr = var->initializer->expr;
      // because of the lazy evaluation nature of this treatment
      // we will treat a reference which is initialized from an extern
      // reference as a constant in the linking stage
      return expr->isConstant();
    }
  }
}

bool ReferenceBinding::isConstant() const {
  auto id = dynamic_cast<const IdExpression*>(from.get());
  if (id) {
    return isConstant(id);
  } else {
    // either binds to a literal value or a temporary; so it's a constant
    return true;
  }
}

SLiteralExpression ReferenceBinding::toConstant() const {
  auto id = dynamic_pointer_cast<const IdExpression>(from);
  if (id) {
    return make_shared<LiteralExpression>(id->getAddress());
  } else {
    // note: the temporary case is implicitly detected this way
    if (auto literalExpr = dynamic_cast<const LiteralExpression*>(from.get())) {
      return make_shared<LiteralExpression>(
               make_shared<LiteralAddressValue>(refType, literalExpr->value));
    } else {
      return make_shared<LiteralExpression>(
               make_shared<TemporaryAddressValue>(refType, getType(), from));
    }
  }
}

void ReferenceBinding::output(ostream& out) const {
  out << format("Reference({})", *from);
}

}
