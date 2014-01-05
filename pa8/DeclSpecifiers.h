#pragma once

#include "common.h"
#include "Type.h"
#include "StorageClass.h"

namespace compiler {

class DeclSpecifiers {
 public:
  void setTypedef();
  void addStorageClass(StorageClass storage);
  void addCvQualifier(CvQualifier cv);
  void addType(SType t, bool isTypeName);
  void finalize();

  SType getType() const {
    CHECK(finalized_);
    return type_;
  }
  bool isTypedef() const { return isTypedef_; }
  StorageClass getStorageClass() const { return storageClass_; }

 private:
  SType type_;
  StorageClass storageClass_ { StorageClass::Unspecified };
  CvQualifier cvQualifier_;
  bool isTypedef_ = false;
  bool finalized_ = false;
  bool hasTypeName_ = false;
};

}
