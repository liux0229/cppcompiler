#pragma once

#include "QuotedLiteralFSM.h"

namespace compiler {

class StringLiteralFSM : public QuotedLiteralFSM
{
protected:
  char getQuote() const override { return '"'; }
  std::vector<std::string> getEncoding() const override {
    return { "u", "U", "u8", "L" };
  }
};

}
