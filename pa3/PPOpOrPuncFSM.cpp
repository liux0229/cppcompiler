#include "PPOpOrPuncFSM.h"
#include "Utf8Encoder.h"
#include "common.h"
#include <string>
#include <vector>
#include <unordered_set>

namespace compiler {

using namespace std;

const vector<string> Operators =
{
  "{",  "}",  "[",  "]",  "#",  "##",  "(",  ")",  "<:",  ":>",  "<%",  "%>",
  "%:", "%:%:",     ";",  ":",  "...", "?",   "::",
  ".",  ".*", "+",  "-",  "*",  "/",   "%",  "^",  "&",   "|",   "~",   "!",
  "=",  "<",  ">",  "+=", "-=", "*=",  "/=", "%=", "^=",  "&=",  "|=",  "<<",
  ">>", ">>=",      "<<=",      "<=",  ">=", "&&",  "==", "!=",  "||",  "++",
  "--", ",",  "->*",      "->",
  // this special string is necessary to handle the <:: special case
  // itself is not a token
  "<::"
};

// See C++ standard 2.13 Operators and punctuators
const unordered_set<string> Digraph =
{
	"new", "delete", "and", "and_eq", "bitand",
	"bitor", "compl", "not", "not_eq", "or",
	"or_eq", "xor", "xor_eq",
};

PPOpOrPuncFSM::PPOpOrPuncFSM()
{
  for (const auto& s : Operators) {
    trie_.insert(s);
  }
  current_ = trie_.root();
}

void PPOpOrPuncFSM::clearInput()
{
  ch_.clear();
  current_ = trie_.root();
  matched_ = 0;
}

StateMachine* PPOpOrPuncFSM::put(int c)
{
  const TrieNode* next = current_->getChild(c);
  if (!next) {
    // cannot match c
    if (current_ != trie_.root()) {
      // we have already matched something
      if (matched_ > 0) {
        if (Utf8Encoder::encode(ch_) == "<::") {
          if (c == ':' || c == '>') {
            send_(PPToken(PPTokenType::PPOpOrPunc, vector<int> { '<', ':' }));
            matched_ = 2;
          } else {
            // special rule
            send_(PPToken(PPTokenType::PPOpOrPunc, vector<int> { '<' }));
            matched_ = 1;
          }
        } else {
          vector<int> m(ch_.begin(), ch_.begin() + matched_);
          send_(PPToken(PPTokenType::PPOpOrPunc, m));
        }
      }

      vector<int> left(ch_.begin() + matched_, ch_.end());
      left.push_back(c);
      clearInput();
      StateMachine* result{nullptr};
      // one assumption is that x in left until c must match the current
      // state machine
      for (int x : left) {
        result = put(x);
      }
      return result;
    } else {
      return nullptr;
    }
  } else {
    // do not accept yet in case we can match further
    current_ = next;
    ch_.push_back(c);
    if (current_->leaf()) {
      matched_ = ch_.size();
    }
    return this;
  }
}

StateMachine* PPOpOrPuncFSM::put(const vector<int>& ch)
{
  // not allowed to call this while we are in the middle of accepting something
  CHECK(current_ == trie_.root());

  string x = Utf8Encoder::encode(ch);
  // std::cout << "transfered: " << x << std::endl;
  if (x == "." ||
      Digraph.find(x) != Digraph.end()) {
    send_(PPToken(PPTokenType::PPOpOrPunc, ch));
    return this;
  } else {
    return nullptr;
  }
}

}
