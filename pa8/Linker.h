#pragma once
#include "TranslationUnit.h"

namespace compiler {

class Linker {
 public:
  using Image = std::vector<char>;
  struct Address : std::pair<size_t, size_t> {
    using Base = std::pair<size_t, size_t>;
    using Base::Base;
    Address() : Base(0, 0) { }
    bool valid() const { return first < second; }
  };

  void addTranslationUnit(UTranslationUnit&& unit);
  Image process();
 private:

  void checkOdr();
  void addExternal(SMember m);
  SMember getExternal(SMember m);
  SMember getUnique(SMember m);
  void generateImage();

  template<size_t N>
  Address gen(const char (&data)[N]) {
    return genEntry(data, N, N);
  }
  Address genEntry(const char* data, size_t n, size_t alignment);
  Address genZero(size_t n, size_t alignment);

  void genHeader();
  Address genFunction();
  Address genFundalmental(SVariableMember m);
  Address genArray(SVariableMember m);
  Address genPointer(SVariableMember m);
  Address genReference(SVariableMember m);

  std::vector<char> getAddress(SMember);
  std::vector<char> getAddress(size_t addr);

  // addr - address of the pointer we need to update later
  void addLiteral(const ConstantValue* literal, Address addr = Address{});

  std::vector<UTranslationUnit> units_;
  std::multimap<std::string, SMember> members_;
  Image image_;
  std::map<SMember, Address> memberAddress_;

  std::vector<const ConstantValue*> literals_;
  std::map<const ConstantValue*, std::vector<Address>> addressToLiteral_;
};

}
