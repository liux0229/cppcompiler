#pragma once

#include "common.h"
#include "PostTokenUtils.h"

namespace compiler {

struct CvQualifier {
  enum Value {
    None = 0x0,
    Const = 0x1,
    Volatile = 0x2
  };

  CvQualifier() { }
  CvQualifier(Value v) : value(v) { }

  bool operator==(const CvQualifier& other) const {
    return value == other.value;
  }

  CvQualifier operator|(const CvQualifier& other) const {
    if (value & other.value) {
      Throw("duplicate cv-qualifier: {x} vs {x}", value, other.value);
    }
    return CvQualifier(static_cast<Value>(value | other.value));
  }

  CvQualifier& operator|=(const CvQualifier& other) {
    *this = *this | other;
    return *this;
  }

  bool isConst() const {
    return value & Const;
  }

  bool isVolatile() const {
    return value & Volatile;
  }

  Value value { None };
};
MakeUnique(CvQualifier);

std::ostream& operator<<(std::ostream& out, const CvQualifier& cvQualifier);

class Type;
MakeShared(Type);

class Type {
 public:
  std::string getName() const;
  virtual bool isReal() const { return true; }
  virtual SType combine(const Type& other) const;
  virtual void setDepended(SType depended) { }
  virtual void setCvQualifier(CvQualifier cvQualifier) {
    cvQualifier_ = cvQualifier;
  }

  virtual void output(std::ostream& out) const = 0;
  virtual SType clone() const = 0;

  // type traits
  virtual bool isVoid() const { return false; }
  virtual bool isPointer() const { return false; }
  virtual bool isReference() const { return false; }
  virtual bool isArray() const { return false; }
  virtual bool isFunction() const { return false; }

  void outputCvQualifier(std::ostream& out) const {
    out << cvQualifier_;
  }
  CvQualifier getCvQualifier() const {
    return cvQualifier_;
  }
 private:
  CvQualifier cvQualifier_;
};

inline std::ostream& operator<<(std::ostream& out, const Type& type) {
  type.output(out);
  return out;
}

// A helper, nonreal type
class CvQualifierType : public Type {
  bool isReal() const override { return false; }
  void output(std::ostream& out) const override { outputCvQualifier(out); }
  SType clone() const override { 
    return std::make_shared<CvQualifierType>(*this); 
  }
  SType combine(const Type& other) const override;
};

class FundalmentalType : public Type {
 public:
  typedef std::vector<ETokenType> TypeSpecifiers;

  FundalmentalType(ETokenType type) : FundalmentalType(TypeSpecifiers{type}) {
  }
  FundalmentalType(const TypeSpecifiers& typeSpecifier);

  EFundamentalType getType() const { return type_; }

  SType combine(const Type& other) const override;
  void output(std::ostream& out) const override;
  SType clone() const override {
    return std::make_shared<FundalmentalType>(*this);
  }
  bool isVoid() const override {
    return type_ == FT_VOID;
  }

 private:
  struct Compare {
    bool operator()(const TypeSpecifiers& a, const TypeSpecifiers& b);
  };
  static std::map<TypeSpecifiers, EFundamentalType, Compare> validCombinations_;
  static std::map<ETokenType, int> typeSpecifierRank_;
  TypeSpecifiers specifiers_;
  // TODO: maybe refactor this out
  EFundamentalType type_;
};

class DependentType : public Type {
 public:
  void setDepended(SType depended) override {
    checkDepended(depended);
    depended_ = depended;
  }
 protected:
  virtual void checkDepended(SType depended) const = 0;
  void outputDepended(std::ostream& out) const;
  SType depended_;
};

class PointerType : public DependentType {
 public:
  SType clone() const override {
    return std::make_shared<PointerType>(*this);
  }

  void output(std::ostream& out) const override;
  bool isPointer() const override { return true; }

 protected:
  void checkDepended(SType depended) const override;
};
MakeShared(PointerType);

class ReferenceType : public DependentType {
 public:
  enum Kind {
    LValueRef,
    RValueRef
  };
  ReferenceType(Kind kind) : kind_(kind) { }

  SType clone() const override {
    return std::make_shared<ReferenceType>(*this);
  }
  void setCvQualifier(CvQualifier cvQualifier) override {
    // cv-qualifiers applied to a reference type is ignored
  }
  void output(std::ostream& out) const override;
  bool isReference() const override { return true; }

 protected:
  void checkDepended(SType depended) const override;

 private:
  Kind kind_;
};

class ArrayType : public DependentType {
 public:
  ArrayType(size_t size) : size_(size) { }

  SType clone() const override {
    return std::make_shared<ArrayType>(*this);
  }
  void setCvQualifier(CvQualifier cvQualifier) override;
  void output(std::ostream& out) const override;
  bool isArray() const override { return true; }

  SPointerType toPointer() const; 

 protected:
  void checkDepended(SType depended) const override;

 private:
  size_t size_;
};

class FunctionType : public DependentType {
 public:
  FunctionType(std::vector<SType>&& params, bool hasVarArgs);

  SType clone() const override {
    return std::make_shared<FunctionType>(*this);
  }
  bool isFunction() const override { return true; }

 protected:
  void checkDepended(SType depended) const override;
  void output(std::ostream& out) const override;

 private:
  std::vector<SType> parameters_;
  bool hasVarArgs_;
};

}
