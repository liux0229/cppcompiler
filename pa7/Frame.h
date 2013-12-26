#pragma once

#include "Namespace.h"
#include <deque>

namespace compiler {

class Frame {
 public:
  Frame() {
    globalNamespace_ = make_unique<Namespace>("", true, false, nullptr);
    openNamespace(&*globalNamespace_);
  }

  const Namespace* getGlobalNamespace() const {
    return &*globalNamespace_;
  }

  Namespace* curNamespace() const {
    return namespaces_.back();
  }

  void openNamespace(Namespace* ns) {
    namespaces_.push_back(ns);
  }

  void closeNamespace() {
    namespaces_.pop_back();
  }

 private:
  UNamespace globalNamespace_;
  std::deque<Namespace*> namespaces_;
};
MakeUnique(Frame);

}
