#include "common.h"
#include "format.h"
#include "Utf8Encoder.h"
#include "StateMachine.h"
#include "PreprocessingToken.h"
#include <iostream>
#include <functional>
#include <memory>
#include <vector>

namespace compiler {

class Tokenizer
{
public:
  Tokenizer();
  void put(int c);
  void sendTo(std::function<void (const PPToken&)> send) {
    for (auto& fsm : fsms_) {
      fsm->sendTo(send);
    }
  }
private:
  void findFsmAndPut(int c);
  void printChar(int c);
  void init(std::unique_ptr<StateMachine>&& fsm);

  std::vector<std::unique_ptr<StateMachine>> fsms_;
  int current_ { -1 };
};

}
