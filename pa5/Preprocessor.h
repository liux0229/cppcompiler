#pragma once
#include <istream>
#include <functional>
#include "PostProcessingToken.h"
#include "PostTokenizer.h"
#include "PostTokenReceiver.h"
#include "PPDirective.h"
#include "SourceReader.h"

namespace compiler {

class PPDirective;
class PostTokenReceiver;

class Preprocessor
{
public:
  Preprocessor(const std::string& source,
               std::ostream& out)
    : out_(out),
      sourceReader_(source) { }

  void process();
private:
  std::ostream& out_;
  SourceReader sourceReader_;
};
}
