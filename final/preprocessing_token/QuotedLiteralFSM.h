#pragma once

#include "StateMachine.h"
#include "common.h"
#include <vector>
#include <string>

namespace compiler {

namespace ppToken {

class QuotedLiteralFSM : public StateMachine
{
public:
  StateMachine* put(int x) override;
  StateMachine* put(const std::vector<int>& ch) override;
  bool inside() const { return !ch_.empty(); }
protected:
  virtual char getQuote() const = 0;
  virtual PPTokenType getTokenType() const = 0;
  virtual std::vector<std::string> getEncoding() const = 0;
private:
  char quote() {
    return quote_ > 0 ? quote_ : (quote_ = getQuote());
  }
  PPTokenType tokenType() {
    return tokenType_ == PPTokenType::Unknown ?
      (tokenType_ = getTokenType()) :
      tokenType_;
  }
  const std::vector<std::string>& encoding() {
    if (encoding_.empty()) {
      encoding_ = getEncoding();
      for (auto& s : encoding_) {
        s += quote();
      }
    }
    return encoding_;
  }
  char quote_ { 0 };
  PPTokenType tokenType_ { PPTokenType::Unknown };
  std::vector<std::string> encoding_;

  bool extend(int x);
  std::vector<int> ch_;
  int octal_ { 0 };
  bool hex_ { false };
};

} // ppToken

} // compiler
