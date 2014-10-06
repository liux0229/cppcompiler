#pragma once

#include "Trie.h"
#include "StateMachine.h"
#include <vector>

namespace compiler {

namespace ppToken {

class PPOpOrPuncFSM : public StateMachine
{
public:
  PPOpOrPuncFSM();
  StateMachine* put(int x) override;
  StateMachine* put(const std::vector<int>& ch) override;
private:
  void clearInput();
  Trie trie_;
  const TrieNode* current_;
  std::vector<int> ch_;
  int matched_ { 0 };
};

} // ppToken

} // compiler
