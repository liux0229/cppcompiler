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
    if (token.getType() != PostTokenType::NewLine) {
      // printToken(token); 
    }
    send_(token);
  }

private:
  void printToken(const PostToken& token) const {
    std::cout << token.toStr() << std::endl;
  }

  std::function<void (const PostToken&)> send_;
};

}
