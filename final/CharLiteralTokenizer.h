#pragma once

#include "preprocessing_token/PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include <string>

namespace compiler {

class CharLiteralTokenizer
{
public:
  using PPToken = ppToken::PPToken;
  CharLiteralTokenizer(TokenReceiver& receiver)
    : receiver_(receiver) {
  }
  void put(const PPToken& token);
private:
  char quote() const { return '\''; }
  void output(const PPToken& token, 
              EFundamentalType type, 
              int codePoint, 
              const std::string& suffix);
  void handle(const PPToken& token, const std::string& suffix = "");
	TokenReceiver& receiver_;
};

}
