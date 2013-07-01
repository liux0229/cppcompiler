#include "CharacterLiteralFSM.h"
#include "Utf8Encoder.h"
#include "common.h"
#include <string>
#include <cctype>
#include <unordered_set>

namespace {

bool isOctal(int x) {
  return x >= '0' && x <= '7';
}

}

namespace compiler {

using namespace std;

const unordered_set<int> SimpleEscape =
{
	'\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
};

bool CharacterLiteralFSM::extend(int x)
{
  CHECK(!ch_.empty());

  bool terminated = false;

  // whether the current sequence can be extended
  if (octal_ > 0 && octal_ < 3 && isOctal(x)) {
    ++octal_;
  } else if (hex_ && isxdigit(x)) {
    /* no-op */
  } else if (ch_.back() == '\\') {
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
    } else if (x == '\'') {
      terminated = true;
    }
  }

  ch_.push_back(x);
  return terminated;
}

StateMachine* CharacterLiteralFSM::put(int x)
{
  // std::cout << "Received: " << char(x) << endl;
  if (ch_.empty()) {
    if (x == '\'') {
      ch_.push_back(x);
      return this;
    }
    return nullptr;
  }
  
  bool terminated = extend(x);
  if (terminated) {
    send_(PPToken(PPTokenType::CharacterLiteral, ch_));
    ch_.clear();
    CHECK(octal_ == 0);
    CHECK(!hex_);
  }
  return this;
}

StateMachine* CharacterLiteralFSM::put(const std::vector<int>& ch)
{
  string x = Utf8Encoder::encode(ch);
  if (x == "u'" || x == "U'" || x == "L'") {
    ch_ = ch;
    return this;
  }
  return nullptr;
}

}
