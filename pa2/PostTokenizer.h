#pragma once

#include "PreprocessingToken.h"
#include "PostTokenWriter.h"
#include "CharLiteralPostTokenizer.h"
#include "StringLiteralPostTokenizer.h"
#include "FloatLiteralPostTokenizer.h"
#include "IntegerLiteralPostTokenizer.h"

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
  FloatLiteralPostTokenizer floatLiteralPT_ {writer_};
  IntegerLiteralPostTokenizer intLiteralPT_ {writer_};
};

}
