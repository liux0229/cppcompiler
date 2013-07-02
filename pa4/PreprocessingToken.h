#pragma once

#include "common.h"
#include "Utf8Encoder.h"
#include <string>
#include <vector>

namespace compiler {

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

  bool isUserDefined() const {
    return type == PPTokenType::UserDefinedCharacterLiteral ||
           type == PPTokenType::UserDefinedStringLiteral;
  }

  bool isQuotedOrUserDefinedLiteral() const {
    return isQuotedLiteral() || isUserDefined();
  }

  bool isWhite() const {
    return type == PPTokenType::WhitespaceSequence;
  }

  bool isNewLine() const {
    return type == PPTokenType::NewLine;
  }

  bool isId() const {
    return type == PPTokenType::Identifier;
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

  std::string dataStrU8() const {
    return Utf8Encoder::encode(data);
  }

  bool operator==(const PPToken& rhs) const {
    return type == rhs.type &&
           data == rhs.data; 
  }
  bool operator!=(const PPToken& rhs) const {
    return !(*this == rhs);
  }

  PPTokenType type { PPTokenType::Unknown };
  std::vector<int> data;
};

}
