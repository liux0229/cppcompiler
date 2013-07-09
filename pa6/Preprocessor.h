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
               std::function<void (const PostToken&)> send)
    : buildEnv_(buildEnv),
      sourceReader_(source),
      send_(send) { }

  void process();
private:
  BuildEnv buildEnv_;
  SourceReader sourceReader_;
  std::function<void (const PostToken&)> send_;
};
}
