#pragma once

#include "PreprocessingToken.h"
#include "PostTokenWriter.h"
#include "CharLiteralPostTokenizer.h"
#include "StringLiteralPostTokenizer.h"

namespace compiler {

class PostTokenizer
{
public:
  void put(const PPToken& token);
private:
  void handleSimpleOrIdentifier(const PPToken& token);
	PostTokenWriter writer_;
  CharLiteralPostTokenizer charLiteralPT_ {writer_};
  StringLiteralPostTokenizer strLiteralPT_ {writer_};
};

}
