#pragma once

#include "common.h"
#include "Type.h"

namespace compiler {

class DeclSpecifiers {
 public:
  enum StorageClass {
    Unspecified = 0x0,
    Static = 0x1,
    ThreadLocal = 0x2,
    Extern = 0x4,
  };
  inline friend StorageClass operator|(StorageClass a, StorageClass b) {
    return static_cast<StorageClass>(+a | +b);
  }

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
