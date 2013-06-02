#pragma once

#include "common.h"
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
  Unknown,
  Total = Unknown
};

struct PPToken
{
  PPToken() { }
  PPToken(PPTokenType _type, std::vector<int> _data = {}) {
    type = _type;
    data.swap(_data);
  }
  const std::string& typeName() const {
    return PPTokenTypes::Names[static_cast<int>(type)]; 
  }

  bool isQuotedLiteral() const {
    return type == PPTokenType::CharacterLiteral ||
           type == PPTokenType::StringLiteral;
  }

  static PPTokenType getUserDefinedFromType(PPTokenType type) {
    if (type == PPTokenType::CharacterLiteral) {
      return PPTokenType::UserDefinedCharacterLiteral;
    } else if (type == PPTokenType::StringLiteral) {
      return PPTokenType::UserDefinedStringLiteral;
    } else {
      CHECK(false);
    }
  }

  PPTokenType getUserDefined() const {
    return getUserDefinedFromType(type);
  }

  PPTokenType type { PPTokenType::Unknown };
  std::vector<int> data;
};
