#pragma once

#include "PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include "PostTokenUtils.h"
#include <vector>
#include <string>

namespace compiler {

class FloatLiteralPostTokenizer
{
public:
  FloatLiteralPostTokenizer(PostTokenReceiver& receiver)
    : receiver_(receiver) {
  }
  bool put(const PPToken& token);
private:
  bool handleUserDefined(
          const PPToken& token,
          std::vector<int>::const_iterator start,
          std::vector<int>::const_iterator end,
          const std::string& udSuffix);
  bool handleFloat(const PPToken& token);
	PostTokenReceiver& receiver_;
};

}
