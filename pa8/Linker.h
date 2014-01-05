#pragma once
#include "TranslationUnit.h"

namespace compiler {

class Linker {
 public:
  using Image = std::vector<char>;

  void addTranslationUnit(UTranslationUnit&& unit);
  Image process();
 private:

  void checkOdr();
  void addExternal(SMember m);
  void generateImage();

  template<size_t N>
  void gen(const char (&data)[N]) {
    genEntry(data, N, N);
  }
  void genEntry(const char* data, size_t n, size_t alignment);
  void genZero(size_t n, size_t alignment);

  void genHeader();
  void genFunction();
  void genFundalmental(SVariableMember m);
  void genArray(SVariableMember m);
  void genPointer(SVariableMember m);
  void genReference(SVariableMember m);

  std::vector<UTranslationUnit> units_;
  std::multimap<std::string, SMember> members_;
  Image image_;
};

}
