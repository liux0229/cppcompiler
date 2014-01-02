#pragma once
#include "TranslationUnit.h"

namespace compiler {

class Linker {
 public:
  void addTranslationUnit(UTranslationUnit&& unit);
  void process();
 private:
  using SMember = Namespace::SMember;

  void checkOdr();
  void addExternal(SMember m);

  std::vector<UTranslationUnit> units_;
  std::multimap<std::string, Namespace::SMember> members_;
};

}
