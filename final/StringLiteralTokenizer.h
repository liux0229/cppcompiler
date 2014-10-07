#pragma once

#include "preprocessing_token/PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include "TokenUtils.h"
#include <vector>
#include <string>

namespace compiler {

class StringLiteralTokenizer {
public:
  using PPToken = ppToken::PPToken;

  StringLiteralTokenizer(TokenReceiver& receiver)
    : receiver_(receiver) {
  }
  void put(const PPToken& token);
  void terminate();
private:
  char quote() const { return '"'; }
  std::string getSource() const;
  void handle();
  void output(EFundamentalType type, 
              int count, 
              const void *data, 
              int size, 
              std::string suffix);
	TokenReceiver& receiver_;
  std::vector<PPToken> tokens_;
};

}
