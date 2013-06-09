#pragma once

#include "common.h"
#include "PostTokenUtils.h"
#include <string>
#include <vector>
#include <memory>

namespace compiler {

struct PostTokenTypes
{
  static const std::vector<std::string> Names;
};

enum class PostTokenType {
  Invalid,
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

  virtual std::string getName() const = 0;
  virtual std::string getDataStr() const { return ""; }

  std::string source;
};

struct PostTokenEof : public PostToken
{
  PostTokenEof() : PostToken("") { }
  std::string toStr() const override { return "eof"; }
  std::string getName() const override { return ""; }
};

struct PostTokenInvalid : public PostToken
{
  PostTokenInvalid(const std::string& _source) : PostToken(_source) { }
  std::string getName() const override { return "invalid"; }
};

struct PostTokenIdentifier : public PostToken
{
  PostTokenIdentifier(const std::string& _source) : PostToken(_source) { }
  std::string getName() const override { return "identifier"; }
};

struct PostTokenSimple : public PostToken
{
  PostTokenSimple(const std::string& _source, ETokenType _type) 
    : PostToken(_source),
      type(_type) { }
  std::string getName() const override { return "simple"; }
  std::string getDataStr() const override {
    return format(" {}", TokenTypeToStringMap.at(type));
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

template<typename T>
struct PostTokenLiteral : public PostToken
{
  PostTokenLiteral(const std::string& _source,
                   EFundamentalType _type, 
                   const T& _data,
                   std::string _udSuffix)
    : PostToken(_source),
      type(_type),
      data(_data),
      udSuffix(_udSuffix) { }

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

  EFundamentalType type;
  T data;
  std::string udSuffix;
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
