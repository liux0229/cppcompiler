#include "Tokenizer.h"
#include "common.h"
#include "StateMachines.h"

namespace compiler {

using namespace std;

Tokenizer::Tokenizer()
{
  init<NewLineFSM>();
  init<EofFSM>();
  auto identifier = init<IdentifierFSM>();
  auto ppNumber = init<PPNumberFSM>();
  auto characterLiteral = init<CharacterLiteralFSM>();
  auto rawStringLiteral = init<RawStringLiteralFSM>();
  auto ppOpOrPunc = init<PPOpOrPuncFSM>();
  init<WhiteSpaceFSM>();
  // must be the last
  init<NonWhiteSpaceCharFSM>();

  identifier->setTransfer({
      ppOpOrPunc,
      characterLiteral,
      rawStringLiteral
  });
  ppNumber->setTransfer({
      ppOpOrPunc
  });
}

template<typename T>
StateMachine* Tokenizer::init()
{
  fsms_.push_back(make_unique<T>());
  return fsms_.back().get();
}

void Tokenizer::put(int c)
{
  // printChar(c);

  StateMachine* r {nullptr};
  if (!current_ ||
      !(r = current_->put(c)) /* current FSM cannot accept c */) {
    findFsmAndPut(c);
  } else {
    current_ = r;
  }
}

void Tokenizer::findFsmAndPut(int c)
{
  for (auto& fsm : fsms_) {
    StateMachine* r = fsm->put(c);
    if (r) {
      // the last FSM (non-whitespace) never changes the current FSM
      current_ = r != fsms_.back().get() ? r : nullptr;
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
