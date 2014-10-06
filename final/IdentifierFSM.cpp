#include "IdentifierFSM.h"
#include "common.h"
#include "Utf8Utils.h"
#include <cctype>

namespace compiler {

namespace ppToken {

using namespace std;

StateMachine* IdentifierFSM::tryTransfer(const vector<int>& ch)
{
  for (StateMachine* s : transfer_) {
    // note: this can be expensive
    StateMachine* r = s->put(ch);
    if (r) {
      return r;
    }
  }
  return nullptr;
}

StateMachine* IdentifierFSM::put(int c)
{
  if (ch_.empty()) {
    if (!Utf8Utils::isIdentifierStart(c)) {
      return nullptr;
    }
  } else {
    if (!Utf8Utils::isIdentifierNonDigit(c) && !isdigit(c)) {
      StateMachine* r { nullptr };
      if (canTransfer_()) {
        if (c == '\'' || c == '"') {
          vector<int> m = ch_;
          m.push_back(c);
          StateMachine* r = tryTransfer(m);
          if (r) {
            ch_.clear();
            return r;
          }
        }

        // c cannot be part of an identifier like op
        r = tryTransfer(ch_);
      }
      if (!r) {
        send_(PPToken(PPTokenType::Identifier, ch_));
      }
      ch_.clear();
      // c cannot be the start of an identifier
      return nullptr;
    }
  }
  ch_.push_back(c);
  return this;
}

} // ppToken

} // compiler
