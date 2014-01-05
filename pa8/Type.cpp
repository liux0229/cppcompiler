#include "Type.h"
#include <sstream>

namespace compiler {

using namespace std;

ostream& operator<<(ostream& out, const CvQualifier& cvQualifier) {
  if (cvQualifier.isConst()) {
    out << "const ";
  }
  if (cvQualifier.isVolatile()) {
    out << "volatile ";
  }
  return out;
}

string Type::getName() const {
  ostringstream oss;
  oss << *this;
  return oss.str();
}

bool Type::operator==(const Type& other) const {
  return cvQualifier_ == other.cvQualifier_;
}

map<FundalmentalType::TypeSpecifiers,
    EFundamentalType, 
    FundalmentalType::Compare>
FundalmentalType::validCombinations_ {
  { { KW_CHAR }, FT_CHAR },
  { { KW_UNSIGNED, KW_CHAR }, FT_UNSIGNED_CHAR },
  { { KW_SIGNED, KW_CHAR }, FT_SIGNED_CHAR },
  { { KW_CHAR16_T }, FT_CHAR16_T },
  { { KW_CHAR32_T }, FT_CHAR32_T },
  { { KW_BOOL }, FT_BOOL },
  { { KW_UNSIGNED }, FT_UNSIGNED_INT },
  { { KW_UNSIGNED, KW_INT }, FT_UNSIGNED_INT },
  { { KW_SIGNED }, FT_INT },
  { { KW_SIGNED, KW_INT }, FT_INT },
  { { KW_INT }, FT_INT },
  { { KW_UNSIGNED, KW_SHORT, KW_INT }, FT_UNSIGNED_SHORT_INT },
  { { KW_UNSIGNED, KW_SHORT }, FT_UNSIGNED_SHORT_INT },
  { { KW_UNSIGNED, KW_LONG, KW_INT }, FT_UNSIGNED_LONG_INT },
  { { KW_UNSIGNED, KW_LONG }, FT_UNSIGNED_LONG_INT },
  { { KW_UNSIGNED, KW_LONG, KW_LONG, KW_INT }, FT_UNSIGNED_LONG_LONG_INT },
  { { KW_UNSIGNED, KW_LONG, KW_LONG }, FT_UNSIGNED_LONG_LONG_INT },
  { { KW_SIGNED, KW_LONG, KW_INT }, FT_LONG_INT },
  { { KW_SIGNED, KW_LONG }, FT_LONG_INT },
  { { KW_SIGNED, KW_LONG, KW_LONG, KW_INT }, FT_LONG_LONG_INT },
  { { KW_SIGNED, KW_LONG, KW_LONG }, FT_LONG_LONG_INT },
  { { KW_LONG, KW_LONG, KW_INT }, FT_LONG_LONG_INT },
  { { KW_LONG, KW_LONG }, FT_LONG_LONG_INT },
  { { KW_LONG, KW_INT }, FT_LONG_INT },
  { { KW_LONG }, FT_LONG_INT },
  { { KW_SIGNED, KW_SHORT, KW_INT }, FT_SHORT_INT },
  { { KW_SIGNED, KW_SHORT }, FT_SHORT_INT },
  { { KW_SHORT, KW_INT }, FT_SHORT_INT },
  { { KW_SHORT }, FT_SHORT_INT },
  { { KW_WCHAR_T }, FT_WCHAR_T },
  { { KW_FLOAT }, FT_FLOAT },
  { { KW_DOUBLE }, FT_DOUBLE },
  { { KW_LONG, KW_DOUBLE }, FT_LONG_DOUBLE },
  { { KW_VOID }, FT_VOID }
};

map<ETokenType, int> FundalmentalType::typeSpecifierRank_ {
  { KW_UNSIGNED, 0 },
  { KW_SIGNED, 0 },

  { KW_SHORT, 1 },
  { KW_LONG, 1 },

  { KW_CHAR, 2 },
  { KW_CHAR16_T, 2 },
  { KW_CHAR32_T, 2 },
  { KW_WCHAR_T, 2 },
  { KW_INT, 2 },
  { KW_BOOL, 2 },
  { KW_FLOAT, 2 },
  { KW_DOUBLE, 2 },
  { KW_VOID, 2 },
};

map<EFundamentalType, size_t> FundalmentalType::Sizes_ {
  { FT_SIGNED_CHAR, 1 },
  { FT_SHORT_INT, 2 },
  { FT_INT, 4 },
  { FT_LONG_INT, 8 },
  { FT_LONG_LONG_INT, 8 },
  { FT_UNSIGNED_CHAR, 1 },
  { FT_UNSIGNED_SHORT_INT, 2 },
  { FT_UNSIGNED_INT, 4 },
  { FT_UNSIGNED_LONG_INT, 8 },
  { FT_UNSIGNED_LONG_LONG_INT, 8 },
  { FT_WCHAR_T, 4 },
  { FT_CHAR, 1 },
  { FT_CHAR16_T, 2 },
  { FT_CHAR32_T, 4 },
  { FT_BOOL, 1 },
  { FT_FLOAT, 4 },
  { FT_DOUBLE, 8 },
  { FT_LONG_DOUBLE, 16 },
  { FT_NULLPTR_T, 8 },
};

bool FundalmentalType::Compare::operator()(const TypeSpecifiers& a, 
                                           const TypeSpecifiers& b) {
  if (a.size() != b.size()) {
    return a.size() < b.size();
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      return a[i] < b[i];
    }
  }
  return false;
}

FundalmentalType::FundalmentalType(const TypeSpecifiers& typeSpecifiers)
  : specifiers_(typeSpecifiers) {
  setType();
}

void FundalmentalType::setType() {
  auto it = validCombinations_.find(specifiers_);
  if (it == validCombinations_.end()) {
    Throw("Not a valid fundalmental type combination: {}", specifiers_);
  }
  type_ = it->second;
}

void FundalmentalType::combine(const FundalmentalType& other) {
  TypeSpecifiers specifiers(specifiers_);
  specifiers_.insert(specifiers_.end(), 
                     other.specifiers_.begin(),
                     other.specifiers_.end());
  auto compare = [](ETokenType a, ETokenType b) -> bool {
    int r1 = typeSpecifierRank_[a];
    int r2 = typeSpecifierRank_[b];
    if (r1 != r2) {
      return r1 < r2;
    } else {
      return a < b;
    }
  };
  sort(specifiers_.begin(), specifiers_.end(), compare);
  setType();
}

size_t FundalmentalType::getSize() const {
  return Sizes_.at(type_);
}

void FundalmentalType::output(ostream& out) const {
  outputCvQualifier(out);
  out << FundamentalTypeToStringMap.at(type_);
}

void DependentType::outputDepended(ostream& out) const {
  if (depended_) {
    out << *depended_;
  } else {
    out << "<unknown>";
  }
}

void PointerType::checkDepended(SType depended) const {
  if (depended->isReference()) {
    Throw("cannot have a pointer to a reference: {}", *depended);
  }
}

void PointerType::output(ostream& out) const {
  outputCvQualifier(out);
  out << "pointer to ";
  outputDepended(out);
}

void ReferenceType::checkDepended(SType depended) const {
  if (depended->isVoid()) {
    Throw("cannot have a reference to void");
  }
}

void ReferenceType::output(ostream& out) const {
  if (kind_ == LValueRef) {
    out << "lvalue-";
  } else {
    out << "rvalue-";
  }
  out << "reference to ";
  outputDepended(out);
}

void ReferenceType::setDepended(SType depended) {
  if (!depended->isReference()) {
    DependentType::setDepended(depended);
  } else {
    // TODO: T& & would crash

    // Note that this implementation does not handle T&&& cases correctly
    // To handle this case we need to pass in an additional flag indicating
    // whether the setDepended operation is for a typedef'd type

    auto& ref = static_cast<ReferenceType&>(*depended);
    Kind collapsed;
    if (kind_ == LValueRef || ref.kind_ == LValueRef) {
      collapsed = LValueRef;
    } else {
      collapsed = RValueRef;
    }
    kind_ = collapsed;
    DependentType::setDepended(ref.depended_);
  }
}

void ArrayType::checkDepended(SType depended) const {
  if (depended->isFunction()) {
    Throw("array element type cannot be a function: {}", *depended);
  }
  if (depended->isReference()) {
    Throw("array element type cannot be a reference: {}", *depended);
  }
  if (depended->isVoid()) {
    Throw("array element type cannot be void");
  }
}

void ArrayType::setCvQualifier(CvQualifier cvQualifier) {
  CHECK(depended_);
  depended_ = depended_->clone();
  depended_->setCvQualifier(cvQualifier);
}

size_t ArrayType::getSize() const {
  return size_ * depended_->getSize();
}

size_t ArrayType::getAlign() const {
  return depended_->getAlign();
}

void ArrayType::output(ostream& out) const {
  out << "array of ";
  if (size_) {
    out << size_ << " ";
  } else {
    out << "unknown bound of ";
  }
  outputDepended(out);
}

SPointerType ArrayType::toPointer() const {
  auto ret = make_shared<PointerType>();
  ret->setDepended(depended_);
  return ret;
}

bool ArrayType::operator==(const Type& rhs) const {
  if (!DependentType::operator==(rhs)) {
    return false;
  }
  auto& other = static_cast<const ArrayType&>(rhs);
  return size_ == other.size_;
}

bool ArrayType::addSizeTo(const ArrayType& other) const {
  if (!DependentType::operator==(other)) {
    return false;
  }
  return size_ && !other.size_;
}

FunctionType::FunctionType(std::vector<SType>&& params, bool hasVarArgs)
  : parameters_(std::move(params)),
    hasVarArgs_(hasVarArgs) {
  // validate and normalize

  if (any_of(parameters_.begin(), 
             parameters_.end(), 
             [](SType type) { return type->isVoid(); })) {
    if (parameters_.size() > 1 || hasVarArgs_) {
      Throw("function parameter types can either be the form (void), "
            "or do not contain void");
    }
    parameters_.clear();
  }

  for (auto& p : parameters_) {
    if (p->isArray()) {
      p = static_cast<ArrayType&>(*p).toPointer();
    } else if (p->isFunction()) {
      auto pointer = make_shared<PointerType>();
      pointer->setDepended(p);
      p = pointer;
    }
    // TODO: need a way to preserve cv-qualifiers for function definition use
    p->setCvQualifier(CvQualifier::None);
  }
}

void FunctionType::checkDepended(SType depended) const {
  if (depended->isArray()) {
    Throw("function cannot return an array: {}", *depended);
  }
  if (depended->isFunction()) {
    Throw("function cannot return a function: {}", *depended);
  }
}

void FunctionType::output(ostream& out) const {
  out << "function of (";
  const char* sep = "";
  for (auto& p : parameters_) {
    out << sep << *p;
    sep = ", ";
  }

  if (hasVarArgs_) {
    out << sep << "...";
  }

  out << ") returning ";
  outputDepended(out);
}

bool FunctionType::sameParameterAndQualifier(const FunctionType& other) const {
  if (parameters_.size() != other.parameters_.size()) {
    return false;
  }
  for (size_t i = 0; i < parameters_.size(); ++i) {
    if (*parameters_[i] != *other.parameters_[i]) {
      return false;
    }
  }
  return hasVarArgs_ == other.hasVarArgs_;
}

bool FunctionType::operator==(const Type& rhs) const {
  if (!DependentType::operator==(rhs)) {
    return false;
  }
  return sameParameterAndQualifier(static_cast<const FunctionType&>(rhs));
}

}
