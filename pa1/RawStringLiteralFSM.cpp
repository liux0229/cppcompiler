#include "RawStringLiteralFSM.h"
#include "Utf8Encoder.h"
#include "Utf8Utils.h"
#include "common.h"
#include <string>

namespace compiler {

using namespace std;

bool RawStringLiteralFSM::extend(int x)
{
  if (!rChar_) {
    if (x == '(') {
      rChar_ = true;
    } else {
      if (Utf8Utils::isWhiteSpace(x) || 
          x == '\\' ||
          x == ')') {
        Throw("Bad d-char {x} after {}", x, Utf8Encoder::encode(ch_));
      }
      dChar_.push_back(x);
    }
  } else {
    if (x == EndOfFile) {
      Throw("Raw string literal not terminated: {}", Utf8Encoder::encode(ch_));
    }
    // we always have enough chars to compare if we are here
    if (x == '"' && ch_.back() == ')') {
      // try to compare the d-char sequence
      int na = ch_.size();
      int nb = dChar_.size();
      bool failed = false;
      for (int i = 0; i < nb; ++i) {
        if (ch_[na - 2 - i] != dChar_[nb - 1 - i]) {
          failed = true;
          break;
        }
      }
      if (!failed) {
        rChar_ = false;
        dChar_.clear();
        return true;
      }
    }
  }
  return false;
}

StateMachine* RawStringLiteralFSM::put(int x) 
{
  if (ch_.empty()) {
    // we never start from here
    return nullptr;
  }
  bool terminated = extend(x);
  ch_.push_back(x);
  if (terminated) {
    send_(PPToken(PPTokenType::StringLiteral, ch_));
    ch_.clear();
  }
  return this;
}

StateMachine* RawStringLiteralFSM::put(const std::vector<int>& ch) 
{
  string x = Utf8Encoder::encode(ch);
  if (x == "u8R\"" ||
      x ==  "uR\"" ||
      x ==  "UR\"" ||
      x ==  "LR\"" ||
      x ==   "R\"") {
    ch_ = ch;
    return this;
  }
  return nullptr;
}

}
