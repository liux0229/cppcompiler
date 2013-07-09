#pragma once

#include "PreprocessingToken.h"
#include "PostTokenReceiver.h"
#include "CharLiteralPostTokenizer.h"
#include "StringLiteralPostTokenizer.h"
#include "FloatLiteralPostTokenizer.h"
#include "IntegerLiteralPostTokenizer.h"

namespace compiler {

class PostTokenizer
{
public:
  explicit PostTokenizer(const PostTokenReceiver& receiver,
                         bool noStrCatForNewLine = false)
    : receiver_(receiver),
      noStrCatForNewLine_(noStrCatForNewLine) { }
  void put(const PPToken& token);
private:
  void handleSimpleOrIdentifier(const PPToken& token);
	PostTokenReceiver receiver_;
  CharLiteralPostTokenizer charLiteralPT_ {receiver_};
  StringLiteralPostTokenizer strLiteralPT_ {receiver_};
  FloatLiteralPostTokenizer floatLiteralPT_ {receiver_};
  IntegerLiteralPostTokenizer intLiteralPT_ {receiver_};

  bool noStrCatForNewLine_;
};

}
