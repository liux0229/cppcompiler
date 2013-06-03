#pragma once

#include "PreprocessingToken.h"
#include "PostTokenWriter.h"
#include "PostTokenUtils.h"
#include <vector>
#include <string>

namespace compiler {

class IntegerLiteralPostTokenizer
{
public:
  IntegerLiteralPostTokenizer(PostTokenWriter& writer)
    : writer_(writer) {
  }
  bool put(const PPToken& token);
private:
  bool handleUserDefined(
          const PPToken& token,
          std::vector<int>::const_iterator start,
          std::vector<int>::const_iterator end,
          const std::string& udSuffix);
  bool handleInteger(const PPToken& token);
	PostTokenWriter& writer_;
};

}
