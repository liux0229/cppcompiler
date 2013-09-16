#pragma once

#include "common.h"
#include "PostTokenUtils.h"
#include <string>
#include <vector>
#include <memory>
#include <type_traits>

namespace compiler {

enum class PostTokenType {
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

struct PostToken
{
  PostToken(const std::string& _source = "") 
    : source(_source) {
  }

  virtual std::string toStr() const {
    return format("{} {}{}", getName(), source, getDataStr());
  }

  // Simple form which is most representative of this PostToken
  // e.g. used for parser output
  virtual std::string toSimpleStr() const {
    return toStr();
  }

  virtual std::string getName() const = 0;
  virtual std::string getDataStr() const { return ""; }
  virtual PostTokenType getType() const = 0;
  virtual std::unique_ptr<PostToken> copy() const = 0;

  // utility functions
  bool isSimple() const { return getType() == PostTokenType::Simple; }
  bool isIdentifier() const { return getType() == PostTokenType::Identifier; }
  bool isNewLine() const { return getType() == PostTokenType::NewLine; }
  bool isLiteral() const { return getType() == PostTokenType::Literal; }
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

typedef std::unique_ptr<PostToken> UToken;

struct PostTokenEof : public PostToken
{
  PostTokenEof() : PostToken("") { }
  std::string toStr() const override { return "eof"; }
  std::string getName() const override { return ""; }
  PostTokenType getType() const override { return PostTokenType::Eof; }
  std::unique_ptr<PostToken> copy() const override {
    return make_unique<PostTokenEof>(*this);
  }
};

struct PostTokenNewLine : public PostToken
{
  PostTokenNewLine() : PostToken("") { }
  std::string toStr() const override { return "<new-line>"; }
  std::string getName() const override { return ""; }
  PostTokenType getType() const override { return PostTokenType::NewLine; }
  std::unique_ptr<PostToken> copy() const override {
    return make_unique<PostTokenNewLine>(*this);
  }
};

struct PostTokenInvalid : public PostToken
{
  PostTokenInvalid(const std::string& _source) : PostToken(_source) { }
  std::string getName() const override { return "invalid"; }
  PostTokenType getType() const override { return PostTokenType::Invalid; }
  std::unique_ptr<PostToken> copy() const override {
    return make_unique<PostTokenInvalid>(*this);
  }
};

struct PostTokenIdentifier : public PostToken
{
  PostTokenIdentifier(const std::string& _source) : PostToken(_source) { }
  std::string getName() const override { return "identifier"; }
  std::string toSimpleStr() const override { return source; }
  PostTokenType getType() const override { return PostTokenType::Identifier; }
  std::unique_ptr<PostToken> copy() const override {
    return make_unique<PostTokenIdentifier>(*this);
  }
};

struct PostTokenSimple : public PostToken
{
  PostTokenSimple(const std::string& _source, ETokenType _type) 
    : PostToken(_source),
      type(_type) { }
  std::string getName() const override { return "simple"; }
  std::string toSimpleStr() const override { 
    return getSimpleTokenTypeName(type);
  }
  std::string getDataStr() const override {
    return format(" {}", getSimpleTokenTypeName(type));
  }
  PostTokenType getType() const override { return PostTokenType::Simple; }
  std::unique_ptr<PostToken> copy() const override {
    return make_unique<PostTokenSimple>(*this);
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
        break;
      default:
        CHECK(false);
        break;
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

struct PostTokenLiteralBase : public PostToken
{
  PostTokenLiteralBase(const std::string& _source,
                       EFundamentalType _type, 
                       std::string _udSuffix)
    : PostToken(_source),
      type(_type),
      udSuffix(_udSuffix) {
  }

  PostTokenType getType() const override { return PostTokenType::Literal; }

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

  virtual std::unique_ptr<PostTokenLiteralBase> promoteTo64() const = 0;

  virtual std::string toIntegralStr() const = 0;

  EFundamentalType type;
  std::string udSuffix;
};

typedef std::unique_ptr<PostTokenLiteralBase> UTokenLiteral;

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
struct PostTokenLiteral : public PostTokenLiteralBase
{
  PostTokenLiteral(const std::string& _source,
                   EFundamentalType _type, 
                   const T& _data,
                   std::string _udSuffix)
    : PostTokenLiteralBase(_source, _type, _udSuffix),
      data(_data) { }

  std::unique_ptr<PostToken> copy() const override {
    return make_unique<PostTokenLiteral<T>>(*this);
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

  std::unique_ptr<PostTokenLiteralBase> promoteTo64() const override {
    CHECK(isIntegral());
    if (std::is_signed<T>::value) {
      return make_unique<PostTokenLiteral<long long>>(
                source, 
                FT_LONG_LONG_INT, 
                ToInteger<T>()(data),
                "");
    } else {
      return make_unique<PostTokenLiteral<unsigned long long>>(
                source, 
                FT_UNSIGNED_LONG_LONG_INT, 
                ToInteger<T>()(data),
                "");
    }
  }

  std::string toIntegralStr() const override {
    CHECK(isIntegral());
    return format(std::is_signed<T>() ? "{}" : "{}u", data);
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

  T data;
};

namespace {

namespace GetPostTokenLiteral {
  template<typename T>
  std::unique_ptr<PostToken> get(const std::string& source,
                                 EFundamentalType type, 
                                 const T& data,
                                 const std::string& udSuffix = "") {
    return make_unique<PostTokenLiteral<T>>(source, 
                                            type, 
                                            data,
                                            udSuffix);
  }
}

} // anoymous

} // compiler
