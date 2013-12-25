#pragma once

#include "common.h"
#include "Type.h"

namespace compiler {

struct DeclSpecifiers {
  void setType(SType t);
  void validate() const;

  SType type;
  // storage class
  // typedef
};

}
