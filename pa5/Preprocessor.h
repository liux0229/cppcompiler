#pragma once
#include <istream>
#include <functional>
#include "PostProcessingToken.h"
#include "PostTokenizer.h"
#include "PostTokenReceiver.h"
#include "PPDirective.h"
#include "SourceReader.h"
#include "BuildEnv.h"

namespace compiler {

class PPDirective;
class PostTokenReceiver;

class Preprocessor
{
public:
  Preprocessor(BuildEnv buildEnv,
               const std::string& source,
               std::ostream& out)
    : buildEnv_(buildEnv),
      out_(out),
      sourceReader_(source) { }

  void process();
private:
  BuildEnv buildEnv_;
  std::ostream& out_;
  SourceReader sourceReader_;
};
}
