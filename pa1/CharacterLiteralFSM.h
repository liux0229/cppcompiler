#pragma once

#include "QuotedLiteralFSM.h"

namespace compiler {

class CharacterLiteralFSM : public QuotedLiteralFSM
{
  // TODO: character literal cannot be empty
protected:
  char getQuote() const override { return '\''; }
  std::vector<std::string> getEncoding() const override {
    return { "u", "U", "L" };
  }
};

}
