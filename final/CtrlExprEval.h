#pragma once

#include "Token.h"
#include <vector>
#include <memory>
#include <functional>

namespace compiler {

class CtrlExprEval
{
public:
  CtrlExprEval(bool printResult, 
               std::function<bool (const std::string&)> isDefined)
    : printResult_(printResult),
      isDefined_(isDefined)  { }
  void put(const Token& token);
  UTokenLiteral getResult() {
    CHECK(hasResult_);
    UTokenLiteral r = move(result_);
    clearResult();
    return r;
  }
private:
  void printResult();
  void clearResult() {
    result_ = nullptr;
    hasResult_ = false;
  }

  std::vector<std::unique_ptr<Token>> tokens_;
  UTokenLiteral result_;
  bool printResult_;
  bool hasResult_ { false };
  std::function<bool (const std::string&)> isDefined_;
};

}
