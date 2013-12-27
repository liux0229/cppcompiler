#include "DeclSpecifiers.h"

namespace compiler {

using namespace std;

void DeclSpecifiers::setTypedef() {
  if (type_ || storageClass_ != Unspecified) {
    Throw("typedef must be the first declSpecifier in declSpecifierSeq");
  }
  isTypedef_ = true;
}

void DeclSpecifiers::addStorageClass(StorageClass storage) {
  if (isTypedef_) {
    Throw("typedef must not be combied with storageClassSpecifier");
  }
  if (storageClass_ & storage) {
    Throw("duplicate storage class specifier: {x}", storage);
  }
  if (storage != ThreadLocal && (storageClass_ & ~ThreadLocal)) {
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
  type_->setCvQualifier(type_->getCvQualifier().combine(cvQualifier_, false));
  finalized_ = true;
}

}
