#pragma once
#include <istream>
#include <functional>
#include "Token.h"
#include "Tokenizer.h"
#include "PostTokenReceiver.h"
#include "PPDirective.h"
#include "SourceReader.h"
#include "BuildEnv.h"

namespace compiler {

class PPDirective;
class TokenReceiver;

class Preprocessor
{
public:
  Preprocessor(BuildEnv buildEnv,
               const std::string& source,
               std::function<void (const Token&)> send)
    : buildEnv_(buildEnv),
      sourceReader_(source),
      send_(send) { }

  void process();
private:
  BuildEnv buildEnv_;
  SourceReader sourceReader_;
  std::function<void (const Token&)> send_;
};
}
