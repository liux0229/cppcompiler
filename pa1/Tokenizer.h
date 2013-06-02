#include "common.h"
#include "format.h"
#include "Utf8Encoder.h"
#include "StateMachine.h"
#include "HeaderNameFSM.h"
#include "PreprocessingToken.h"
#include "IncludeDetector.h"
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
  void sendTo(std::function<void (const PPToken&)> send);
  bool insideRawString() const;
  bool insideQuotedLiteral() const;
private:
  template<typename T> StateMachine* init();
  void findFsmAndPut(int c);
  void printChar(int c);
  StateMachine* init(std::unique_ptr<StateMachine>&& fsm);

  std::vector<std::unique_ptr<StateMachine>> fsms_;
  HeaderNameFSM headerNameFsm_;
  StateMachine* current_ { nullptr };
  std::function<void (const PPToken&)> send_;
  IncludeDetector includeDetector_;
};

}
