#pragma once

#include "PreprocessingToken.h"
#include "PostTokenWriter.h"
#include <string>

namespace compiler {

class CharLiteralPostTokenizer
{
public:
  CharLiteralPostTokenizer(PostTokenWriter& writer)
    : writer_(writer) {
  }
  void put(const PPToken& token);
private:
  char quote() const { return '\''; }
  void output(const PPToken& token, 
              EFundamentalType type, 
              int codePoint, 
              const std::string& suffix);
  void handle(const PPToken& token, const std::string& suffix = "");
	PostTokenWriter& writer_;
};

}
