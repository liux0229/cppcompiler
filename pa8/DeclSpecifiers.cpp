#include "DeclSpecifiers.h"

namespace compiler {

using namespace std;

void DeclSpecifiers::setTypedef() {
  if (type_ || 
      storageClass_ != StorageClass::Unspecified ||
      isConstExpr_) {
    Throw("typedef must be the first declSpecifier in declSpecifierSeq");
  }
  isTypedef_ = true;
}

void DeclSpecifiers::setConstExpr() {
  if (isTypedef_) {
    Throw("unexpected constexpr in typedef");
  }
  isConstExpr_ = true;
}

void DeclSpecifiers::addStorageClass(StorageClass storage) {
  if (isTypedef_) {
    Throw("typedef must not be combied with storageClassSpecifier");
  }
  if (storageClass_ & storage) {
    Throw("duplicate storage class specifier: {x}", storage);
  }
  if (storage != StorageClass::ThreadLocal && 
      (storageClass_ & ~StorageClass::ThreadLocal)) {
    Throw("incompatible storage class specifiers: {x} vs {x}", 
           storage, 
           storageClass_); 
  }
  storageClass_ = storageClass_ | storage;
}

void DeclSpecifiers::addCvQualifier(CvQualifier cv) {
  cvQualifier_ = cvQualifier_.combine(cv, true); 
}

void DeclSpecifiers::addType(SType t, bool isTypeName) {
  if (!type_) {
    type_ = t;
  } else {
    if (hasTypeName_ || isTypeName) {
      Throw("DeclSpecifiers cannot contain both typeName and fundalmental type"
            "{} vs {}",
            *type_,
            *t);
    }
    CHECK(type_->isFundalmental() && t->isFundalmental());
    auto exist = static_cast<FundalmentalType*>(type_.get());
    auto add = static_cast<FundalmentalType*>(t.get());
    exist->combine(*add);
  }
}

void DeclSpecifiers::finalize() {
  if (!type_) {
    Throw("DeclSpecifiers do not have a type associated with it");
  }
  // the cv-qualifiers associated with the type can be duplicated with
  // the cv-qualifiers in the DeclSpecifierSeq
  auto cv = type_->getCvQualifier().combine(cvQualifier_, false);
  if (cv != type_->getCvQualifier()) {
    // need to create a new type object
    type_ = type_->clone();
    type_->setCvQualifier(cv);
  }

  if (isConstExpr_) {
    type_ = type_->clone();
    type_->setCvQualifier(CvQualifier::Const);
  }

  finalized_ = true;
}

}
