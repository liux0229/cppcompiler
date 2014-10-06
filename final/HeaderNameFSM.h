#pragma once

#include "StateMachine.h"
#include "common.h"
#include "Utf8Encoder.h"
#include <vector>
#include <string>

namespace compiler {

namespace ppToken {

class HeaderNameFSM : public StateMachine
{
public:
  StateMachine* put(int x) override {
    if (ch_.empty()) {
      if (x == '<' || x == '"') {
        ch_.push_back(x);
        return this;
      }
      return nullptr;
    }

    if (x == '\n') {
      Throw("Header name not terminated before new line: {}",
            Utf8Encoder::encode(ch_));
    } else if ((x == '>' && ch_[0] == '<') ||
               (x == '"' && ch_[0] == '"')) {
      ch_.push_back(x);
      send_(PPToken(PPTokenType::HeaderName, ch_));
      ch_.clear();
    } else {
      // any other ch's is just part of the header name
      ch_.push_back(x);
    }

    return this;
  }
  bool inside() const {
    return !ch_.empty();
  }
private:
  std::vector<int> ch_;
};

} // ppToken

} // compiler
