#pragma once
#include "Type.h"
#include "PostTokenUtils.h"

namespace compiler {

template<typename T> struct FundalmentalValue;
template<typename T> 
  using UFundalmentalValue = std::unique_ptr<FundalmentalValue<T>>;

struct ConstantValue;
MakeUnique(ConstantValue);
struct ConstantValue {
  template<typename T>
  static UFundalmentalValue<T> 
  createFundalmentalValue(EFundamentalType ft, const T& d);

  ConstantValue(SType t) : type(t) { }
  virtual UConstantValue to(EFundamentalType target) const {
    Throw("cannot convert to fundalmental type {}", target);
    return nullptr;
  }

  virtual std::vector<char> toBytes() const { return {}; }

  SType type;
};

template<typename T>
struct FundalmentalValue : ConstantValue {
  FundalmentalValue(SType type, const T& d)
    : ConstantValue(type), data(d) {
  }
  FundalmentalValue(EFundamentalType ft, const T& d)
    : FundalmentalValue(std::make_shared<FundalmentalType>(ft), d) {
  }

  template<typename U>
  UConstantValue convert(EFundamentalType target) const {
    return make_unique<FundalmentalValue<U>>(target, 
                                       static_cast<U>(data));
  }

  UConstantValue to(EFundamentalType target) const override {
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

  std::vector<char> toBytes() const override { 
    std::vector<char> ret;
    auto buf = reinterpret_cast<const char*>(&data);
    for (size_t i = 0; i < type->getSize(); ++i) {
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
UFundalmentalValue<T>
ConstantValue::createFundalmentalValue(EFundamentalType ft, const T& d) {
  return make_unique<FundalmentalValue<T>>(ft, d);
}

template<typename T>
struct ArrayValue : ConstantValue {
  ArrayValue(SArrayType type, const std::vector<T>& d)
    : ConstantValue(type),
      data(d) {
  }
  std::vector<T> data;
};

struct Member;
using SMember = std::shared_ptr<Member>;
struct MemberPointerValue : ConstantValue {
  SMember member;
};

struct ConstantPointerValue : ConstantValue {
  UConstantValue v;
};

}
