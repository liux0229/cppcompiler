#pragma once

#include "common.h"
#include <map>
#include <memory>
#include <utility>

namespace compiler {

class TrieNode;

class TrieNode {
public:
  TrieNode* getOrCreateChild(int c) {
    auto it = next_.find(c);
    if (it != next_.end()) {
      return it->second.get();
    } else {
      it = next_.insert(make_pair(c, make_unique<TrieNode>())).first;
      return it->second.get();
    }
  }
  const TrieNode* getChild(int c) const {
    auto it = next_.find(c);
    return it == next_.end() ? nullptr : it->second.get();
  }
  bool& leaf() { return leaf_; }
  bool leaf() const { return leaf_; }
private:
  std::map<int, std::unique_ptr<TrieNode>> next_;
  bool leaf_;
};

class Trie {
public:
  const TrieNode* root() const {
    return &root_;
  }
  void insert(const std::string& s) {
    TrieNode* n = &root_;
    for (char c : s) {
      n = n->getOrCreateChild(c);
    }
    n->leaf() = true;
  }
private:
  TrieNode root_;
};

}
