#pragma once

#include "common.h"
#include "PostTokenUtils.h"
#include "ConstantValue.h"
#include <string>
#include <vector>
#include <memory>
#include <type_traits>

namespace compiler {

enum class TokenType {
  Invalid,
  NewLine,
  Simple,
  Identifier,
  Literal,
  LiteralArray,
  UserDefinedLiteralCharacter,
  UserDefinedLiteralString,
  UserDefinedLiteralInteger,
  UserDefinedLiteralFloat,
  Eof
};

struct Token
{
  Token(const std::string& _source = "") 
    : source(_source) {
  }

  virtual std::string toStr() const {
    return format("{} {}{}", getName(), source, getDataStr());
  }

  // Simple form which is most representative of this Token
  // e.g. used for parser output
  virtual std::string toSimpleStr() const {
    return toStr();
  }

  virtual std::string getName() const = 0;
  virtual std::string getDataStr() const { return ""; }
  virtual TokenType getType() const = 0;
  virtual std::unique_ptr<Token> copy() const = 0;

  // utility functions
  bool isSimple() const { return getType() == TokenType::Simple; }
  bool isIdentifier() const { return getType() == TokenType::Identifier; }
  bool isNewLine() const { return getType() == TokenType::NewLine; }
  bool isLiteral() const { return getType() == TokenType::Literal; }
  bool isEof() const { return getType() == TokenType::Eof; }
  // TODO: we can argue whether to put this here or in a more specific place
  bool isEmptyStr() const {
    // isLiteral check should be redundant, but for clarity
    return isLiteral() && 
           source == "\"\"";
  }
  bool isZero() const {
    // isLiteral check should be redundant, but for clarity
    return isLiteral() && source == "0";
  }

  std::string source;
};

typedef std::unique_ptr<Token> UToken;

struct TokenEof : public Token
{
  TokenEof() : Token("") { }
  std::string toStr() const override { return "eof"; }
  std::string getName() const override { return ""; }
  TokenType getType() const override { return TokenType::Eof; }
  std::unique_ptr<Token> copy() const override {
    return make_unique<TokenEof>(*this);
  }
};

struct TokenNewLine : public Token
{
  TokenNewLine() : Token("") { }
  std::string toStr() const override { return "<new-line>"; }
  std::string getName() const override { return ""; }
  TokenType getType() const override { return TokenType::NewLine; }
  std::unique_ptr<Token> copy() const override {
    return make_unique<TokenNewLine>(*this);
  }
};

struct TokenInvalid : public Token
{
  TokenInvalid(const std::string& _source) : Token(_source) { }
  std::string getName() const override { return "invalid"; }
  TokenType getType() const override { return TokenType::Invalid; }
  std::unique_ptr<Token> copy() const override {
    return make_unique<TokenInvalid>(*this);
  }
};

struct TokenIdentifier : public Token
{
  TokenIdentifier(const std::string& _source) : Token(_source) { }
  std::string getName() const override { return "identifier"; }
  std::string toSimpleStr() const override { return source; }
  TokenType getType() const override { return TokenType::Identifier; }
  std::unique_ptr<Token> copy() const override {
    return make_unique<TokenIdentifier>(*this);
  }
};

struct TokenSimple : public Token
{
  TokenSimple(const std::string& _source, ETokenType _type) 
    : Token(_source),
      type(_type) { }
  std::string getName() const override { return "simple"; }
  std::string toSimpleStr() const override { 
    return getSimpleTokenTypeName(type);
  }
  std::string getDataStr() const override {
    return format(" {}", getSimpleTokenTypeName(type));
  }
  TokenType getType() const override { return TokenType::Simple; }
  std::unique_ptr<Token> copy() const override {
    return make_unique<TokenSimple>(*this);
  }

  ETokenType type;
};

template<typename T>
struct TypedHexDump {
  std::string operator()(const T& data) {
    return HexDump(&data, sizeof(T));
  }
};

template<>
struct TypedHexDump<std::string> {
  std::string operator()(const std::string& data) {
    return HexDump(data.data(), sizeof(data[0]) * data.size());
  }
};

template<typename T>
struct TypedHexDump<std::vector<T>> {
  std::string operator()(const std::vector<T>& data) {
    return HexDump(&data[0], sizeof(T) * data.size());
  }
};

template<typename T>
struct GetTypePrefixStr {
  std::string operator()(const T& data, EFundamentalType type) {
    return "";
  }
};

template<>
struct GetTypePrefixStr<std::string> {
  std::string operator()(const std::string& data, EFundamentalType type) {
    if (type == FT_INT || type == FT_DOUBLE) {
      // special output rule for user defined integer or double
      return "";
    } else {
      return format(" array of {}", data.size());
    }
  }
};

template<typename T>
struct GetTypePrefixStr<std::vector<T>> {
  std::string operator()(const std::vector<T>& data, EFundamentalType type) {
    return format(" array of {}", data.size());
  }
};

template<typename T>
struct GetTypeSpecifier {
  std::string operator()(EFundamentalType type) {
    switch (type) {
      case FT_CHAR:
      case FT_CHAR16_T:
      case FT_CHAR32_T:
      case FT_WCHAR_T:
        return "character";
      default:
        CHECK(false);
        return "";
    }
  }
};

template<>
struct GetTypeSpecifier<std::string> {
  std::string operator()(EFundamentalType type) {
    // user-defined literals for integer and float also 
    // have data of type string
    switch (type) {
      case FT_INT:
        return "integer";
        break;
      case FT_DOUBLE:
        return "floating";
        break;
      default:
        return "string";
        break;
    }
  }
};

template<typename T>
struct GetTypeSpecifier<std::vector<T>> {
  std::string operator()(EFundamentalType type) {
    return "string";
  }
};

template<typename T>
struct LiteralTypeTraits {
  using TElement = T;

  SType getType(const T&) const {
    return std::make_shared<FundalmentalType>(FundamentalTypeOf<T>());
  }

  // not used
  std::vector<TElement> getArrayValue(const T&) const { return {}; }
};

template<>
struct LiteralTypeTraits<std::string> {
  using TElement = char;

  SType getType(const std::string& v) const {
    auto type = std::make_shared<ArrayType>(v.size());
    auto element = LiteralTypeTraits<char>().getType(char{});
    element->setCvQualifier(CvQualifier::Const);
    type->setDepended(element);
    return type;
  }

  std::vector<TElement> getArrayValue(const std::string& d) const {
    return std::vector<TElement>(d.begin(), d.end());
  }
};

template<typename T>
struct LiteralTypeTraits<std::vector<T>> {
  using TElement = T;

  SType getType(const std::vector<T>& v) const {
    auto type = std::make_shared<ArrayType>(v.size());
    auto element = LiteralTypeTraits<T>().getType(T{});
    element->setCvQualifier(CvQualifier::Const);
    type->setDepended(element);
    return type;
  }

  std::vector<TElement> getArrayValue(const std::vector<TElement>& d) const {
    return d;
  }
};

namespace {

template<typename T>
std::string typedHexDump(const T& data) {
  return TypedHexDump<T>()(data);
}

template<typename T>
std::string getTypePrefixStr(const T& data, EFundamentalType type) {
  return GetTypePrefixStr<T>()(data, type);
}

} // anonymous

struct TokenLiteralBase : public Token
{
  TokenLiteralBase(const std::string& _source,
                       EFundamentalType _type, 
                       std::string _udSuffix)
    : Token(_source),
      type(_type),
      udSuffix(_udSuffix) {
  }

  TokenType getType() const override { return TokenType::Literal; }

  bool isUserDefined() const {
    return !udSuffix.empty();
  }

  // we could define these according to type,
  // but using type traits in the derived class makes these simpler
  virtual bool isIntegral() const = 0;
  virtual bool isSigned() const = 0;

  virtual bool isIntegralZero() const = 0;

  virtual long long toSigned64() const = 0;
  virtual unsigned long long toUnsigned64() const = 0;

  virtual std::unique_ptr<TokenLiteralBase> promoteTo64() const = 0;

  virtual std::string toIntegralStr() const = 0;

  virtual SConstantValue toConstantValue() const = 0;

  EFundamentalType type;
  std::string udSuffix;
};

typedef std::unique_ptr<TokenLiteralBase> UTokenLiteral;

template<typename T, typename E = void>
struct ToInteger {
  long long operator()(const T&) {
    Throw("Try to convert a non-integral type to integer: {}",
          typeid(T).name());
    return 0;
  }
};

template<typename T>
struct ToInteger<T,
                 typename std::enable_if<
                    std::is_integral<T>::value && std::is_signed<T>::value
                 >::type> {
  long long operator()(const T& v) {
    return (long long)v;
  }
};

template<typename T>
struct ToInteger<T,
                 typename std::enable_if<
                    std::is_integral<T>::value && !std::is_signed<T>::value
                 >::type> {
  unsigned long long operator()(const T& v) {
    return (unsigned long long)v;
  }
};

template<typename T>
struct TokenLiteral : public TokenLiteralBase
{
  TokenLiteral(const std::string& _source,
                   EFundamentalType _type, 
                   const T& _data,
                   std::string _udSuffix)
    : TokenLiteralBase(_source, _type, _udSuffix),
      data(_data) { }

  std::unique_ptr<Token> copy() const override {
    return make_unique<TokenLiteral<T>>(*this);
  }
  // TODO: find a more elegant way of expressing this
  std::unique_ptr<TokenLiteral<T>> typedCopy() const {
    return make_unique<TokenLiteral<T>>(*this);
  }

  bool isIntegral() const override {
    return !isUserDefined() && std::is_integral<T>::value;
  }

  bool isSigned() const override {
    CHECK(isIntegral());
    return std::is_signed<T>::value;
  }

  long long toSigned64() const override {
    // Do not allow cast unsigned to signed
    CHECK(isSigned());
    return (long long)ToInteger<T>()(data);
  }

  unsigned long long toUnsigned64() const override {
    // Do not allow cast unsigned to signed
    CHECK(isIntegral());
    return (unsigned long long)ToInteger<T>()(data);
  }

  bool isIntegralZero() const override {
    return toUnsigned64() == 0ULL;
  }

  std::unique_ptr<TokenLiteralBase> promoteTo64() const override {
    CHECK(isIntegral());
    if (std::is_signed<T>::value) {
      return make_unique<TokenLiteral<long long>>(
                source, 
                FT_LONG_LONG_INT, 
                ToInteger<T>()(data),
                "");
    } else {
      return make_unique<TokenLiteral<unsigned long long>>(
                source, 
                FT_UNSIGNED_LONG_LONG_INT, 
                ToInteger<T>()(data),
                "");
    }
  }

  std::string toIntegralStr() const override {
    CHECK(isIntegral());
    return format(std::is_signed<T>::value ? "{}" : "{}u", data);
  }

  std::string getName() const override { 
    if (udSuffix.empty()) {
      return "literal"; 
    } else {
      return "user-defined-literal";
    }
  }

  std::string getUserDefinedExtraTypeSpecifier() const {
    if (udSuffix.empty()) {
      return "";
    } else {
      return format(" {}", GetTypeSpecifier<T>()(type));
    }
  }
  
  bool userDefinedIntegerOrFloats() const {
    return !udSuffix.empty() && (type == FT_INT || type == FT_DOUBLE);
  }

  std::string getFundamentalTypeStr() const {
    if (userDefinedIntegerOrFloats()) {
      // special output rule for user defined integers or floats
      return "";
    } else {
      return format(" {}", FundamentalTypeToStringMap.at(type));
    }
  }

  std::string getDump() const {
    if (userDefinedIntegerOrFloats()) {
      // make this compile for any type of data
      return format(" {}", data);
    } else {
      return format(" {}", typedHexDump(data));
    }
  }

  std::string getDataStr() const override
  {
    return format("{}{}{}{}{}", 
                  udSuffix.empty() ? "" : format(" {}", udSuffix),
                  getUserDefinedExtraTypeSpecifier(),
                  getTypePrefixStr(data, type),
                  getFundamentalTypeStr(),
                  getDump());
  }

  SConstantValue toConstantValue() const override {
    CHECK(udSuffix.empty());
    LiteralTypeTraits<T> traits;
    auto type = traits.getType(data);
    if (type->isFundalmental()) {
      return std::make_shared<FundalmentalValue<T>>(
               type->toFundalmental(), 
               data); 
    } else {
      CHECK(type->isArray());
      using ET = typename LiteralTypeTraits<T>::TElement;
      return std::make_shared<ArrayValue<ET>>(
               type->toArray(), 
               traits.getArrayValue(data));
    }
  }

  T data;
};

namespace {

namespace GetTokenLiteral {
  template<typename T>
  std::unique_ptr<TokenLiteral<T>> get(const std::string& source,
                                 EFundamentalType type, 
                                 const T& data,
                                 const std::string& udSuffix = "") {
    return make_unique<TokenLiteral<T>>(source, 
                                            type, 
                                            data,
                                            udSuffix);
  }
}

ETokenType getSimpleTokenType(const Token& token) {
  CHECK(token.isSimple());
  return static_cast<const TokenSimple&>(token).type;
}

} // anoymous

} // compiler
