#include "Tokenizer.h"
#include "common.h"
#include "NewLineFSM.h"
#include "EofFSM.h"

namespace compiler {

using namespace std;

Tokenizer::Tokenizer()
{
  init(make_unique<NewLineFSM>());
  init(make_unique<EofFSM>());
}

void Tokenizer::init(unique_ptr<StateMachine>&& fsm)
{
  fsms_.push_back(move(fsm));
}

void Tokenizer::put(int c)
{
  printChar(c);

  if (current_ == -1 ||
      !fsms_[current_]->put(c) /* current FSM cannot accept c */) {
    findFsmAndPut(c);
  }
}

void Tokenizer::findFsmAndPut(int c)
{
  int i;
  for (i = 0; i < static_cast<int>(fsms_.size()); ++i) {
    auto& fsm = fsms_[i];
    if (fsm->put(c)) {
      current_ = i;
      break;
    }
  }
  // CHECK(i < fsms_.size());
}

void Tokenizer::printChar(int c)
{
  if (c == EndOfFile) {
    std::cout << "EOF" << std::endl;
  } else {
    std::cout 
      << format("token:{} {x}", Utf8Encoder::encode(c), c) 
      << std::endl;
  }
}

}
