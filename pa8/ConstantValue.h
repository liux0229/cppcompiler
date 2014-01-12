#pragma once
#include "Type.h"
#include "PostTokenUtils.h"

namespace compiler {

template<typename T> struct FundalmentalValue;
template<typename T> 
  using SFundalmentalValue = std::shared_ptr<FundalmentalValue<T>>;

struct ConstantValue;
MakeUnique(ConstantValue);
MakeShared(ConstantValue);
struct ConstantValue {
  template<typename T>
  static SFundalmentalValue<T> 
  createFundalmentalValue(EFundamentalType ft, const T& d);

  template<typename T>
  static SFundalmentalValue<T> 
  createFundalmentalValue(const T& d);

  ConstantValue(SType t) : type(t) { }
  virtual SConstantValue to(EFundamentalType target) const {
    Throw("cannot convert to fundalmental type {}", target);
    return nullptr;
  }

  // TODO: make pure virtual
  virtual std::vector<char> toBytes() const { return {}; }

  SType type;
};

struct FundalmentalValueBase : ConstantValue {
  using ConstantValue::ConstantValue;
  virtual bool isZero() const = 0;
};

template<typename T>
struct FundalmentalValue : FundalmentalValueBase {
  FundalmentalValue(SType type, const T& d)
    : FundalmentalValueBase(type), data(d) {
  }
  FundalmentalValue(EFundamentalType ft, const T& d)
    : FundalmentalValue(std::make_shared<FundalmentalType>(ft), d) {
  }

  template<typename U>
  std::shared_ptr<FundalmentalValue<U>>
  convert(EFundamentalType target) const {
    return std::make_shared<FundalmentalValue<U>>(
             target, 
             static_cast<U>(data));
  }

  SConstantValue to(EFundamentalType target) const override {
    switch (target) {
      case FT_SIGNED_CHAR:
        return convert<signed char>(target);
        break;
      case FT_SHORT_INT:
        return convert<short>(target);
        break;
      case FT_INT:
        return convert<int>(target);
        break;
      case FT_LONG_INT:
        return convert<long>(target);
        break;
      case FT_LONG_LONG_INT:
        return convert<long long>(target);
        break;
      case FT_UNSIGNED_CHAR:
        return convert<unsigned char>(target);
        break;
      case FT_UNSIGNED_SHORT_INT:
        return convert<unsigned short>(target);
        break;
      case FT_UNSIGNED_INT:
        return convert<unsigned>(target);
        break;
      case FT_UNSIGNED_LONG_INT:
        return convert<unsigned long>(target);
        break;
      case FT_UNSIGNED_LONG_LONG_INT:
        return convert<unsigned long long>(target);
        break;
      case FT_WCHAR_T:
        return convert<wchar_t>(target);
        break;
      case FT_CHAR:
        return convert<char>(target);
        break;
      case FT_CHAR16_T:
        return convert<char16_t>(target);
        break;
      case FT_CHAR32_T:
        return convert<char32_t>(target);
        break;
      case FT_BOOL:
        return convert<bool>(target);
        break;
      case FT_FLOAT:
        return convert<float>(target);
        break;
      case FT_DOUBLE:
        return convert<double>(target);
        break;
      case FT_LONG_DOUBLE:
        return convert<long double>(target);
        break;
      default:
        MCHECK(false, format("Invalid target type: {}", target));
        return nullptr;
        break;
    }
  }

  bool isZero() const override {
    return data == 0;
  }

  std::vector<char> toBytes() const override { 
    std::vector<char> ret;
    auto buf = reinterpret_cast<const char*>(&data);
    for (size_t i = 0; i < type->getTypeSize(); ++i) {
      ret.push_back(buf[i]);
    }
    return ret;
  }

  T data;
};

template<>
struct FundalmentalValue<std::nullptr_t> : ConstantValue {
  FundalmentalValue(SFundalmentalType type, std::nullptr_t)
    : ConstantValue(type) {
  }
  FundalmentalValue(EFundamentalType ft, std::nullptr_t d)
    : FundalmentalValue(std::make_shared<FundalmentalType>(ft), d) {
  }
};

// TODO: remove these
template<>
struct FundalmentalValue<std::string> : ConstantValue {
  FundalmentalValue(SFundalmentalType type, std::string)
    : ConstantValue(type) {
  }
};
template<typename T>
struct FundalmentalValue<std::vector<T>> : ConstantValue {
  FundalmentalValue(SFundalmentalType type, std::vector<T>)
    : ConstantValue(type) {
  }
};

template<typename T>
SFundalmentalValue<T>
ConstantValue::createFundalmentalValue(EFundamentalType ft, const T& d) {
  return std::make_shared<FundalmentalValue<T>>(ft, d);
}

template<typename T>
SFundalmentalValue<T>
ConstantValue::createFundalmentalValue(const T& d) {
  return std::make_shared<FundalmentalValue<T>>(FundamentalTypeOf<T>(), d);
}

struct ArrayValueBase : ConstantValue { 
  using ConstantValue::ConstantValue;
};

template<typename T>
struct ArrayValue : ArrayValueBase {
  ArrayValue(SArrayType type, const std::vector<T>& d)
    : ArrayValueBase(type),
      data(d) {
  }

  std::vector<char> toBytes() const override { 
    std::vector<char> ret;
    for (auto& e : data) {
      auto eb = createFundalmentalValue(e)->toBytes();
      ret.insert(ret.end(), eb.begin(), eb.end());
    }
    return ret; 
  }

  std::vector<T> data;
};

struct LiteralExpression;
using SLiteralExpression = std::shared_ptr<const LiteralExpression>;
struct AddressValue : ConstantValue {
  using ConstantValue::ConstantValue;

  // whether the pointed-to is constant
  virtual bool isConstant() const = 0;

  // get-constant from the pointed-to
  virtual SLiteralExpression toConstant() const = 0;
};
MakeShared(AddressValue);

struct Member;
using SMember = std::shared_ptr<Member>;
struct MemberAddressValue : AddressValue {
  // type is the type of the entity which represents this address
  MemberAddressValue(SType type, SMember m) 
    : AddressValue(type),
      member(m) { }

  bool isConstant() const override;
  SLiteralExpression toConstant() const override;

  SMember member;
};

struct LiteralAddressValue : AddressValue {
  // type is the type of the entity which represents this address
  LiteralAddressValue(SType type, SConstantValue v) 
    : AddressValue(type),
      literal(v) { 
  }

  bool isConstant() const override {
    return true;
  }
  SLiteralExpression toConstant() const override;

  SConstantValue literal;
};

struct Expression;
using SExpression = std::shared_ptr<const Expression>;
struct VariableMember;
using SVariableMember = std::shared_ptr<VariableMember>;
struct TemporaryAddressValue : AddressValue {
  // type is the type of the entity which represents this address
  TemporaryAddressValue(SType type, 
                        SType temporaryType, 
                        SExpression initExpr);

  bool isConstant() const override;
  SLiteralExpression toConstant() const override;

  // a member which does not belong to any namespace
  SVariableMember member;
};

}
