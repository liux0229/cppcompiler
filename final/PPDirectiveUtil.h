#pragma once
#include "PreprocessingToken.h"

namespace compiler {

inline bool isPound(const ppToken::PPToken& token) {
  return token.type == ppToken::PPTokenType::PPOpOrPunc && 
         (token.dataStrU8() == "#" ||
          token.dataStrU8() == "%:");
}

inline bool isDoublePound(const ppToken::PPToken& token) {
  return token.type == ppToken::PPTokenType::PPOpOrPunc &&
         (token.dataStrU8() == "##" ||
          token.dataStrU8() == "%:%:");
}

inline bool isLParen(const ppToken::PPToken& token) {
  return token.type == ppToken::PPTokenType::PPOpOrPunc &&
         token.dataStrU8() == "(";
}

inline bool isRParen(const ppToken::PPToken& token) {
  return token.type == ppToken::PPTokenType::PPOpOrPunc &&
         token.dataStrU8() == ")";
}

inline bool isEllipse(const ppToken::PPToken& token) {
  return token.type == ppToken::PPTokenType::PPOpOrPunc &&
         token.dataStrU8() == "...";
}

inline bool isComma(const ppToken::PPToken& token) {
  return token.type == ppToken::PPTokenType::PPOpOrPunc &&
         token.dataStrU8() == ",";
}

inline bool isIdentifier(const ppToken::PPToken& token, const std::string& identifier) {
  return token.isId() && token.dataStrU8() == identifier;
}

inline size_t skipWhite(const std::vector<ppToken::PPToken>& tokens, size_t i) {
  while (i < tokens.size() && tokens[i].isWhite()) {
    ++i;
  }
  return i;
}

inline std::vector<int> toVector(const std::string& s)
{
  std::vector<int> r;
  r.reserve(s.size());
  for (char c : s) {
    r.push_back(static_cast<unsigned char>(c));
  }
  return r;
}

inline std::vector<int> stringify(const std::string& s)
{
  return toVector("\"" + s + "\"");
}

} // compiler
