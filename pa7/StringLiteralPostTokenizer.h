#pragma once

#include "PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include "PostTokenUtils.h"
#include <vector>
#include <string>

namespace compiler {

class StringLiteralPostTokenizer
{
public:
  StringLiteralPostTokenizer(PostTokenReceiver& receiver)
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
	PostTokenReceiver& receiver_;
  std::vector<PPToken> tokens_;
};

}
