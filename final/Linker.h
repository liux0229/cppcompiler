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
  Address genVariable(SVariableMember m);
  Address genFundalmental(SVariableMember m);
  Address genArray(SVariableMember m);
  Address genPointer(SVariableMember m);
  Address genReference(SVariableMember m);
  void update(Address target, const std::vector<char>& bytes);

  bool getAddress(SMember, std::vector<char>& ret);
  std::vector<char> getAddress(size_t addr);

  // addr - address of the pointer/reference we need to update later
  void addLiteral(const ConstantValue* literal, Address addr = Address{});
  void addTemporary(SVariableMember m, Address addr);

  std::vector<UTranslationUnit> units_;
  std::multimap<std::string, SMember> members_;
  Image image_;
  std::map<SMember, Address> memberAddress_;
  // addresses that need to be updated after the member is laid down
  std::map<SMember, std::vector<Address>> addressToMember_;

  std::vector<const ConstantValue*> literals_;
  // addresses that need to be updated after the literal is laid down
  std::map<const ConstantValue*, std::vector<Address>> addressToLiteral_;

  std::vector<SVariableMember> temporary_;
  // addresses that need to be updated after the temporary is laid down
  std::map<SVariableMember, std::vector<Address>> addressToTemporary_;
};

}
