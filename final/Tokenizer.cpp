#include "Tokenizer.h"
#include "common.h"
#include "StateMachines.h"

namespace compiler {

using namespace std;

PPTokenizerHelper::PPTokenizerHelper()
{
  init<NewLineFSM>();
  init<EofFSM>();
  auto identifier = init<IdentifierFSM>();
  auto ppNumber = init<PPNumberFSM>();
  auto characterLiteral = init<CharacterLiteralFSM>();
  auto rawStringLiteral = init<RawStringLiteralFSM>();
  auto stringLiteral = init<StringLiteralFSM>();
  auto ppOpOrPunc = init<PPOpOrPuncFSM>();
  init<WhiteSpaceFSM>();
  // must be the last
  init<NonWhiteSpaceCharFSM>();

  identifier->setCanTransfer([this]() {
    return pToken_.type != PPTokenType::StringLiteral &&
           pToken_.type != PPTokenType::CharacterLiteral;
  });
  identifier->setTransfer({
      ppOpOrPunc,
      characterLiteral,
      rawStringLiteral,
      stringLiteral
  });
  ppNumber->setTransfer({
      ppOpOrPunc
  });
}

template<typename T>
T* PPTokenizerHelper::init()
{
  fsms_.push_back(make_unique<T>());
  return static_cast<T* >(fsms_.back().get());
}

void PPTokenizerHelper::sendTo(function<void(const PPToken&)> send) {
  send_ = send;
  auto s = [this](const PPToken& token) {
    pToken_ = token;
    includeDetector_.put(token);
    send_(token);
  };
  for (auto& fsm : fsms_) {
    fsm->sendTo(s);
  }
  headerNameFsm_.sendTo(s);
}

void PPTokenizerHelper::put(int c)
{
  // printChar(c);

  if (current_) {
    // need to return two values from put to simplify this
    if (current_ == &headerNameFsm_ && !headerNameFsm_.inside()) {
      current_ = nullptr;
    } else {
      current_ = current_->put(c); 
    }
  }
  if (!current_) {
    findFsmAndPut(c);
  }
}

void PPTokenizerHelper::findFsmAndPut(int c)
{
  // only invoke HeaderNameFSM when the context is right
  if (includeDetector_.canMatchHeader()) {
    current_ = headerNameFsm_.put(c);
    if (current_) {
      return;
    }
  }

  for (auto& fsm : fsms_) {
    StateMachine* r = fsm->put(c);
    if (r) {
      // the last FSM (non-whitespace) never changes the current FSM
      current_ = r != fsms_.back().get() ? r : nullptr;
      break;
    }
  }
}

void PPTokenizerHelper::printChar(int c)
{
  if (c == EndOfFile) {
    std::cout << "EOF" << std::endl;
  } else {
    std::cout 
      << format("token:{} {x}", Utf8Encoder::encode(c), c) 
      << std::endl;
  }
}

bool PPTokenizerHelper::insideRawString() const
{
  auto s = dynamic_cast<RawStringLiteralFSM*>(current_);
  return s != nullptr && s->inside();
}

bool PPTokenizerHelper::insideQuotedLiteral() const
{
  auto s = dynamic_cast<QuotedLiteralFSM*>(current_);
  return s != nullptr && s->inside();
}

}
