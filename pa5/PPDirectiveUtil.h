#pragma once
#include "PreprocessingToken.h"

namespace compiler {
namespace {
bool isPound(const PPToken& token) {
  return token.type == PPTokenType::PPOpOrPunc && 
         (token.dataStrU8() == "#" ||
          token.dataStrU8() == "%:");
}
bool isDoublePound(const PPToken& token) {
  return token.type == PPTokenType::PPOpOrPunc && 
         (token.dataStrU8() == "##" ||
          token.dataStrU8() == "%:%:");
}
bool isLParen(const PPToken& token) {
  return token.type == PPTokenType::PPOpOrPunc && 
         token.dataStrU8() == "(";
}
bool isRParen(const PPToken& token) {
  return token.type == PPTokenType::PPOpOrPunc && 
         token.dataStrU8() == ")";
}
bool isEllipse(const PPToken& token) {
  return token.type == PPTokenType::PPOpOrPunc && 
         token.dataStrU8() == "...";
}
bool isComma(const PPToken& token) {
  return token.type == PPTokenType::PPOpOrPunc && 
         token.dataStrU8() == ",";
}
bool isIdentifier(const PPToken& token, const std::string& identifier) {
  return token.isId() && token.dataStrU8() == identifier;
}
size_t skipWhite(const std::vector<PPToken>& tokens, size_t i) {
  while (i < tokens.size() && tokens[i].isWhite()) {
    ++i;
  }
  return i;
}

} }
