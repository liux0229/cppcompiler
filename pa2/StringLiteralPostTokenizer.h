#pragma once

#include "PreprocessingToken.h"
#include "PostTokenWriter.h"
#include "PostTokenUtils.h"
#include <vector>
#include <string>

namespace compiler {

class StringLiteralPostTokenizer
{
public:
  StringLiteralPostTokenizer(PostTokenWriter& writer)
    : writer_(writer) {
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
	PostTokenWriter& writer_;
  std::vector<PPToken> tokens_;
};

}
