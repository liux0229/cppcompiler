#pragma once
#include "PostProcessingToken.h"
#include <vector>

namespace compiler
{

class Parser
{
public:
  Parser(const std::vector<UToken>& tokens)
    : tokens_(tokens) { }
  bool process();
private:
  const std::vector<UToken>& tokens_;
};

}
