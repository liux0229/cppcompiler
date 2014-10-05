#pragma once

#include "PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include "CharLiteralPostTokenizer.h"
#include "StringLiteralPostTokenizer.h"
#include "FloatLiteralPostTokenizer.h"
#include "IntegerLiteralPostTokenizer.h"

namespace compiler {

class Tokenizer
{
public:
  explicit Tokenizer(const TokenReceiver& receiver,
                         bool noStrCatForNewLine = false)
    : receiver_(receiver),
      noStrCatForNewLine_(noStrCatForNewLine) { }
  void put(const PPToken& token);
private:
  void handleSimpleOrIdentifier(const PPToken& token);
	TokenReceiver receiver_;
  CharLiteralTokenizer charLiteralPT_ {receiver_};
  StringLiteralTokenizer strLiteralPT_ {receiver_};
  FloatLiteralTokenizer floatLiteralPT_ {receiver_};
  IntegerLiteralTokenizer intLiteralPT_ {receiver_};

  bool noStrCatForNewLine_;
};

}
