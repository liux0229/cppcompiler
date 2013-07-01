#pragma once

#include "PostProcessingToken.h"
#include <vector>
#include <memory>

namespace compiler {

class CtrlExprEval
{
public:
  void put(const PostToken& token);
private:
  std::vector<std::unique_ptr<PostToken>> tokens_;
};

}
