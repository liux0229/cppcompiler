#pragma once

#include "Token.h"

namespace compiler {

namespace parser {

class Parser {
public:
  Parser(std::vector<UToken>&& tokens) : tokens_(std::move(tokens)) {}
  void parse();
private:
  std::vector<UToken> tokens_;
};

} // parser

} // compiler