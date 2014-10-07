#pragma once

#include "TokenUtils.h"
#include "Token.h"
#include <iostream>
#include <functional>

namespace compiler {

class TokenReceiver
{
public:
  TokenReceiver(
    const std::function<void (const Token&)>& send = [](const Token&){}) 
    : send_(send) {
  }

  void put(const Token& token) {
    // debug(token, true);
    send_(token);
  }

private:
  void printToken(const Token& token) const {
    std::cout << token.toStr() << std::endl;
  }

  void debug(const Token& token, bool printNewLine) const {
    if (token.getType() != TokenType::NewLine || printNewLine) {
      printToken(token); 
    }
  }

  std::function<void (const Token&)> send_;
};

}
