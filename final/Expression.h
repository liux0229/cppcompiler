#pragma once
#include "common.h"
#include "ConstantValue.h"

namespace compiler {

enum class ValueCategory {
  LValue,
  XValue,
  PRValue,
};

struct Member;
using SMember = std::shared_ptr<Member>;

struct Expression;
using SExpression = std::shared_ptr<const Expression>;
struct LiteralExpression;
using SLiteralExpression = std::shared_ptr<const LiteralExpression>;

struct Expression : std::enable_shared_from_this<Expression> {
  bool isGLValue() const {
    return valueCategory() == ValueCategory::LValue || 
           valueCategory() == ValueCategory::XValue; 
  }
  bool isPRValue() const {
    return valueCategory() == ValueCategory::PRValue;
  }

  virtual bool isConstant() const { return false; }
  virtual ValueCategory valueCategory() const = 0;
  virtual SType getType() const = 0;

  // Whether this expression can be assigned to a variable of the specified type
  // If true return the expr that can be used for the assignment; the expr
  // may contain additional conversions as necessary
  // If false then return nullptr
  SExpression assignableTo(SType target) const;

  // Reduce this expression to a literal expression
  virtual SLiteralExpression toConstant() const {
    Throw("Expression must be a constant");
    return nullptr;
  }

  virtual void output(std::ostream& out) const = 0;
};

inline std::ostream& operator<<(std::ostream& out, const Expression& e) {
  e.output(out);
  return out;
}

struct LiteralExpression : Expression {
  LiteralExpression(SConstantValue v) : value(v) { }

  bool isConstant() const override {
    return true;
  }

  ValueCategory valueCategory() const override {
    return ValueCategory::PRValue;
  }

  SType getType() const override {
    return value->type;
  }

  SLiteralExpression toConstant() const override {
    return std::static_pointer_cast<const LiteralExpression>(
             shared_from_this());
  }

  void output(std::ostream& out) const override {
    // TODO: enhance this
    out << "<LiteralExpression>";
  }

  SConstantValue value;
};

struct IdExpression : Expression {
  IdExpression(SMember e);

  ValueCategory valueCategory() const override {
    return ValueCategory::LValue;
  }

  SType getType() const override;
  bool isConstant() const override;
  SLiteralExpression toConstant() const override;
  SAddressValue getAddress() const;

  void output(std::ostream& out) const override;

  SMember entity;
};

struct ConversionExpression : Expression {
  ConversionExpression(const std::string& n, SExpression f) 
    : name(n), from(f) { }

  ValueCategory valueCategory() const override {
    return ValueCategory::PRValue;
  }

  SType getType() const override { return type; }
  bool isConstant() const override {
    return from->isConstant();
  }

  SType fromType() const { return from->getType(); }

  void output(std::ostream& out) const override {
    out << name << "(" << *from << ")";
  }

  const std::string name;
  SExpression from;
  SType type;
};

struct LValueToRValueConversion : ConversionExpression {
  LValueToRValueConversion(SExpression e)
      : ConversionExpression("LValueToRValueConversion", e) {
    CHECK(from->isGLValue());
    CHECK(!fromType()->isFunction() && !fromType()->isArray());
    type = fromType()->clone();
    type->setCvQualifier(CvQualifier::None);    
  }
  
  SLiteralExpression toConstant() const override {
    return from->toConstant();
  }
};

struct ArrayToPointerConversion : ConversionExpression {
  ArrayToPointerConversion(SExpression e)
      : ConversionExpression("ArrayToPointerConversion", e) {
    CHECK(fromType()->isArray());
    type = fromType()->toArray()->toPointer();
  }
  
  // In the current feature set, we always consider the result of such a
  // conversion to be a constant
  bool isConstant() const override {
    return true;
  }

  SLiteralExpression toConstant() const override;
};

struct FunctionToPointerConversion : ConversionExpression {
  FunctionToPointerConversion(SExpression e)
      : ConversionExpression("FunctionToPointerConversion", e) {
    CHECK(fromType()->isFunction());
    type = std::make_shared<PointerType>();
    type->setDepended(fromType());
  }
  
  bool isConstant() const override {
    return true;
  }

  SLiteralExpression toConstant() const override;
};

struct FundalmentalTypeConversion : ConversionExpression {
  FundalmentalTypeConversion(SExpression e, SFundalmentalType target) 
      : ConversionExpression("FundalmentalTypeConversion", e) {
    CHECK(e->valueCategory() == ValueCategory::PRValue);
    CHECK(e->getType()->isFundalmental() && 
          allowed(e->getType()->toFundalmental(), target));

    type = target->clone();
    type->setCvQualifier(CvQualifier::None);
  }

  static bool allowed(SFundalmentalType from, SFundalmentalType to);

  SLiteralExpression toConstant() const override;
};

// Only covers the conversions not covered by FundalmentalTypeConversion
struct BooleanConversion;
MakeShared(BooleanConversion);
struct BooleanConversion : ConversionExpression {
  static SBooleanConversion create(SExpression e);
  BooleanConversion(SExpression e)
    : ConversionExpression("BooleanConversion", e) {
  }
  SLiteralExpression toConstant() const override;
};

struct PointerConversion : ConversionExpression {
  PointerConversion(SExpression e, SPointerType target) 
    : ConversionExpression("PointerConversion", e) {
    CHECK(e->valueCategory() == ValueCategory::PRValue);
    CHECK(allowed(e)); 

    // no modifications to the type; even to the top level constness
    type = target;
  }

  bool isConstant() const override {
    return true;
  }

  SLiteralExpression toConstant() const override {
    return std::make_shared<LiteralExpression>(
             make_unique<LiteralAddressValue>(getType(), nullptr));
  }

  static bool allowed(SExpression from);
};

struct QualificationConversion : ConversionExpression {
  QualificationConversion(SExpression e, SPointerType target) 
    : ConversionExpression("QualificationConversion", e) {
    CHECK(e->valueCategory() == ValueCategory::PRValue);
    CHECK(e->getType()->isPointer() &&
          allowed(e->getType()->toPointer(), target));

    type = target;
  }

  static bool allowed(SPointerType from, SPointerType to);

  SLiteralExpression toConstant() const override {
    return from->toConstant();
  }
};

struct ReferenceBinding;
MakeShared(ReferenceBinding);
struct ReferenceBinding : Expression {
  static SReferenceBinding create(SExpression e, SReferenceType target);
  ReferenceBinding(SExpression f, SReferenceType t)
    : from(f),
      refType(t) {
  }

  ValueCategory valueCategory() const { return ValueCategory::LValue; }

  SType getType() const override { return refType->getDepended(); }
  bool isConstant() const override;
  bool isConstant(const IdExpression* id) const;
  SLiteralExpression toConstant() const override;
  void output(std::ostream& out) const override;

  SExpression from;
  SReferenceType refType;
};

}
