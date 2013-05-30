#pragma once

#include <string>
#include <vector>

struct PPTokenTypes
{
  static const std::vector<std::string> Names;
};


enum class PPTokenType {
  WhitespaceSequence,
  NewLine,
  HeaderName,
  Identifier,
  PPNumber,
  CharacterLiteral,
  UserDefinedCharacterLiteral,
  StringLiteral,
  UserDefinedStringLiteral,
  PPOpOrPunc,
  NonWhitespaceChar,
  Eof,
  // Sentinel
  NotInitialized,
  Total = NotInitialized
};

struct PPToken
{
  PPToken(PPTokenType _type, std::string _data = "") {
    type = _type;
    data.swap(_data);
  }
  const std::string& typeName() const {
    return PPTokenTypes::Names[static_cast<int>(type)]; 
  }
  PPTokenType type { PPTokenType::NotInitialized };
  std::string data;
};
