#include "DeclSpecifiers.h"

namespace compiler {

using namespace std;

void DeclSpecifiers::setType(SType t) {
  if (!type) {
    type = t;
  } else {
    type = type->combine(*t);
  }
}

void DeclSpecifiers::validate() const {
  if (!type) {
    Throw("DeclSpecifiers does not have a type associated with it");
  }
  if (!type->isReal()) {
    Throw("<{}> is not a real type", *type);
  }
}

}
