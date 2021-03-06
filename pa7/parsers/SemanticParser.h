#pragma once
#include "parsers/ParserCommon.h"
#include "PostProcessingToken.h"
#include "Frame.h"
#include <vector>

namespace compiler
{

class SemanticParser
{
public:
  SemanticParser(const std::vector<UToken>& tokens, const ParserOption& option)
    : tokens_(tokens),
      option_(option) { }
  UFrame process();
private:
  const std::vector<UToken>& tokens_;
  const ParserOption option_;
};

}
