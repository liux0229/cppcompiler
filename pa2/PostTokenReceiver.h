#pragma once

#include "PostTokenUtils.h"
#include "PostProcessingToken.h"
#include <iostream>

namespace compiler {

struct PostTokenReceiver
{
  void put(const PostToken& token) {
    printToken(token);    
  }

  void printToken(const PostToken& token) const {
    std::cout << token.toStr() << std::endl;
  }
};

}
