#pragma once

#include "PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include "PostTokenUtils.h"
#include <vector>
#include <string>

namespace compiler {

class IntegerLiteralPostTokenizer
{
public:
  IntegerLiteralPostTokenizer(PostTokenReceiver& writer)
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
	PostTokenReceiver& writer_;
};

}
