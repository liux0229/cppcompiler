#include "QuotedLiteralFSM.h"
#include "Utf8Encoder.h"
#include "common.h"
#include <string>
#include <cctype>
#include <unordered_set>
#include <algorithm>
#include <vector>

namespace compiler {

namespace ppToken {

using namespace std;

namespace {

bool isOctal(int x) {
  return x >= '0' && x <= '7';
}

bool lastIsBackSlash(const vector<int>& ch)
{
  int i;
  for (i = static_cast<int>(ch.size()) - 1; i >= 0; i--) {
    if (ch[i] != '\\') {
      break;
    }
  }
  CHECK(i >= 0);
  return (ch.size() - 1 - i) % 2 == 1;
}

}

const unordered_set<int> SimpleEscape =
{
  '\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
};

bool QuotedLiteralFSM::extend(int x)
{
  CHECK(!ch_.empty());

  bool terminated = false;

  // whether the current sequence can be extended
  if (octal_ > 0 && octal_ < 3 && isOctal(x)) {
    ++octal_;
  } else if (hex_ && isxdigit(x)) {
    /* no-op */
  } else if (lastIsBackSlash(ch_)) {
    if (isOctal(x)) {
      octal_ = 1;
    } else if (x == 'x') {
      hex_ = true;
    } else if (SimpleEscape.find(x) == SimpleEscape.end()) {
      Throw("Bad escape sequence for {}: {}",
            static_cast<char>(x),
            Utf8Encoder::encode(ch_));
    }
    /*
    else {
    std::cout << format("Escape sequence for {}: {}",
    static_cast<char>(x),
    Utf8Encoder::encode(ch_)) << std::endl;
    }
    */
  } else {
    // cannot extend previous escape sequence
    octal_ = 0;
    hex_ = false;

    if (x == '\n') {
      Throw("Unexpected new line after `{}`", Utf8Encoder::encode(ch_));
    } else if (x == quote()) {
      terminated = true;
    }
  }

  ch_.push_back(x);
  return terminated;
}

StateMachine* QuotedLiteralFSM::put(int x)
{
  // std::cout << "Received: " << char(x) << endl;
  if (ch_.empty()) {
    if (x == quote()) {
      ch_.push_back(x);
      return this;
    }
    return nullptr;
  }

  bool terminated = extend(x);
  if (terminated) {
    send_(PPToken(tokenType(), ch_));
    ch_.clear();
    CHECK(octal_ == 0);
    CHECK(!hex_);
  }
  return this;
}

StateMachine* QuotedLiteralFSM::put(const std::vector<int>& ch)
{
  string x = Utf8Encoder::encode(ch);
  for (auto& s : encoding()) {
    if (x == s) {
      ch_ = ch;
      return this;
    }
  }
  return nullptr;
}

} // ppToken

} // compiler
