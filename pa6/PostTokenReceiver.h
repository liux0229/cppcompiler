#pragma once

#include "PostTokenUtils.h"
#include "PostProcessingToken.h"
#include <iostream>
#include <functional>

namespace compiler {

class  PostTokenReceiver
{
public:
  PostTokenReceiver(
    const std::function<void (const PostToken&)>& send = [](const PostToken&){}) 
    : send_(send) {
  }

  void put(const PostToken& token) {
    // debug(token, true);
    send_(token);
  }

private:
  void printToken(const PostToken& token) const {
    std::cout << token.toStr() << std::endl;
  }

  void debug(const PostToken& token, bool printNewLine) const {
    if (token.getType() != PostTokenType::NewLine || printNewLine) {
      printToken(token); 
    }
  }

  std::function<void (const PostToken&)> send_;
};

}
