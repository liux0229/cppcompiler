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

SType Type::combine(const Type& other) const {
  Throw("{} cannot be combined with {}", *this, other);
  return nullptr;
}

SType CvQualifierType::combine(const Type& other) const {
  auto ret = other.clone();
  ret->setCvQualifier(getCvQualifier() | ret->getCvQualifier());
  return ret;
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
  : specifiers_(typeSpecifiers),
    type_(validCombinations_.at(specifiers_)) {
}

SType FundalmentalType::combine(const Type& that) const {
  if (dynamic_cast<const CvQualifierType*>(&that)) {
    return that.combine(*this);
  }

  auto pOther = dynamic_cast<const FundalmentalType*>(&that);
  if (!pOther) {
    // TODO: display individual type specifiers instead
    Throw("{} cannot be combined with {}", *this, that);
  }
  auto& other = *pOther; 
  TypeSpecifiers specifiers(specifiers_);
  specifiers.insert(specifiers.end(), 
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
  sort(specifiers.begin(), specifiers.end(), compare);
  // TODO: throw with a better error message
  auto ret = make_shared<FundalmentalType>(specifiers);
  ret->setCvQualifier(getCvQualifier() | that.getCvQualifier());
  return ret;
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

}
