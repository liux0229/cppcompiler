#pragma once

#include "common.h"
#include "Type.h"
#include "StorageClass.h"

namespace compiler {

class DeclSpecifiers {
 public:
  void setTypedef();
  void setConstExpr();
  void setInline();
  void addStorageClass(StorageClass storage);
  void addCvQualifier(CvQualifier cv);
  void addType(SType t, bool isTypeName);
  void finalize();

  SType getType() const {
    CHECK(finalized_);
    return type_;
  }
  bool isTypedef() const { return isTypedef_; }
  bool isConstExpr() const { return isConstExpr_; }
  bool isInline() const { return isInline_; }
  StorageClass getStorageClass() const { return storageClass_; }

 private:
  SType type_;
  StorageClass storageClass_ { StorageClass::Unspecified };
  CvQualifier cvQualifier_;
  bool isTypedef_ = false;
  bool isConstExpr_ = false;
  bool isInline_ = false;
  bool finalized_ = false;
  bool hasTypeName_ = false;
};

}
