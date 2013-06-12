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
    debug(token);
    if (token.getType() != PostTokenType::Eof) {
      send_(token);
    } else {
      printToken(token);
    }
  }

private:
  void printToken(const PostToken& token) const {
    std::cout << token.toStr() << std::endl;
  }

  void debug(const PostToken& token) const {
    if (token.getType() != PostTokenType::NewLine) {
      printToken(token); 
    }
  }

  std::function<void (const PostToken&)> send_;
};

}
