#include "PPNumberFSM.h"
#include "Utf8Utils.h"
#include "common.h"
#include <cctype>
#include <vector>

namespace compiler {

namespace ppToken {

using namespace std;

bool PPNumberFSM::canExtend(int c) const
{
  if (isdigit(c) || Utf8Utils::isIdentifierNonDigit(c)) {
    return true;
  } else if (c == '+' || c == '-') {
    CHECK(ch_.size() > 0);
    return ch_.back() == 'E' || ch_.back() == 'e';
  } else {
    return c == '.';
  }
}

StateMachine* PPNumberFSM::put(int c)
{
  CHECK(!transfer_.empty());
  StateMachine* t = transfer_[0];

  if (ch_.size() == 0) {
    if (isdigit(c) || c == '.') {
      ch_.push_back(c);
      return this;
    } else {
      return nullptr;
    }
  } else if (ch_.size() == 1 && ch_[0] == '.') {
    if (!isdigit(c)) {
      if (c != '.') {
        t->put(ch_[0]);
        ch_.clear();
        return t->put(c);
      } else {
        // '..': we don't know how to parse this until the next character
        // so accepting for now
        ch_.push_back(c);
        return this;
      }
    } else {
      ch_.push_back(c);
      return this;
    }
  } else if (ch_.size() == 2 && ch_[0] == '.' && ch_[1] == '.') {
    if (isdigit(c)) {
      // note we must call this overload to let the
      // target fsm know it should not wait for more input
      // also note the use of explicit ctor
      t->put(vector < int > { ch_[0] });

      ch_.erase(ch_.begin());
      ch_.push_back(c);
      return this;
    } else {
      // we cannot process the three characters, run the loop for the outer
      // layer
      t->put(ch_[0]);
      t->put(ch_[1]);
      ch_.clear();
      return t->put(c);
    }
  }

  // already has a pp-number accepted, see if we can extend it
  if (canExtend(c)) {
    ch_.push_back(c);
    return this;
  } else {
    CHECK(ch_.size() > 0);
    send_(PPToken(PPTokenType::PPNumber, ch_));
    ch_.clear();
    return nullptr;
  }
}

} // ppToken

} // compiler
