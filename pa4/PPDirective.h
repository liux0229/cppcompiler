#pragma once
#include "PreprocessingToken.h"
#include "MacroProcessor.h"
#include <functional>
#include <vector>

namespace compiler {

class PPDirective
{
public:
  PPDirective(std::function<void (const PPToken&)> send)
    : send_(send) { }
  void put(const PPToken& token);
private:
  void handleDirective();
  void handleExpand();

  std::function<void (const PPToken&)> send_;
  std::vector<PPToken> text_;
  std::vector<PPToken> directive_;
  MacroProcessor macroProcessor_;
  int state_ { 0 };
};

}
