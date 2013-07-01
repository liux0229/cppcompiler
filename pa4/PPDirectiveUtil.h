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
} }
