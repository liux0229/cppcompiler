#pragma once

#include "Namespace.h"
#include <deque>

namespace compiler {

class TranslationUnit {
 public:
  using VMember = std::vector<SMember>;
  using Frame = size_t;

  TranslationUnit() {
    globalNamespace_ = make_unique<Namespace>("", true, false, nullptr, this);
    openNamespace(&*globalNamespace_);
  }

  Namespace* getGlobalNamespace() const {
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

  Frame saveFrame() const {
    return namespaces_.size();
  }

  void restoreFrame(Frame frame) {
    CHECK(namespaces_.size() >= frame);
    while (namespaces_.size() > frame) {
      closeNamespace();
    }
  }

  void addMember(SMember m) {
    members_.push_back(m);
  }
  const VMember& getVariablesFunctions() const {
    return members_;
  }

 private:
  UNamespace globalNamespace_;
  std::deque<Namespace*> namespaces_;

  // members of this translation unit in declaration order
  VMember members_;
};
MakeUnique(TranslationUnit);

}
