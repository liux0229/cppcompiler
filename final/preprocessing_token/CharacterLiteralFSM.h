#pragma once

#include "QuotedLiteralFSM.h"

namespace compiler {

namespace ppToken {

class CharacterLiteralFSM : public QuotedLiteralFSM
{
  // TODO: character literal cannot be empty
protected:
  char getQuote() const override { return '\''; }
  PPTokenType getTokenType() const override {
    return PPTokenType::CharacterLiteral;
  }
  std::vector<std::string> getEncoding() const override {
    return { "u", "U", "L" };
  }
};

} // ppToken

} // compiler
