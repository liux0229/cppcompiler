#pragma once

#include "Decoder.h"
#include "Tokenizer.h"
#include "PreprocessingToken.h"
#include "common.h"

#include <vector>
#include <memory>
#include <functional>

namespace compiler {

namespace ppToken {

class PPTokenizer
{
public:
  template<typename T> void init();
  PPTokenizer();
  void process(int c);
  void receivedChar(int c);
  bool canMergeIntoUserDefined(const PPToken& token) const;
  void receivedToken(const PPToken& token);
  void sendTo(std::function<void(const PPToken&)> send) {
    send_ = send;
  }

private:
  std::vector<std::unique_ptr<Decoder>> decoders_;
  PPTokenizerHelper tokenizer_;
  int pCh_ { -1 };
  PPToken pToken_;
  std::function<void(const PPToken&)> send_;
};

} // ppToken

} // compiler
