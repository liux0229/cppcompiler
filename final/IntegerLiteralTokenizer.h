#pragma once

#include "PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include "TokenUtils.h"
#include <vector>
#include <string>

namespace compiler {

class IntegerLiteralTokenizer
{
public:
  using PPToken = ppToken::PPToken;

  IntegerLiteralTokenizer(TokenReceiver& receiver)
    : receiver_(receiver) {
  }
  bool put(const PPToken& token);
private:
  bool handleUserDefined(
          const PPToken& token,
          std::vector<int>::const_iterator start,
          std::vector<int>::const_iterator end,
          const std::string& udSuffix);
  bool handleInteger(const PPToken& token);
	TokenReceiver& receiver_;
};

}
