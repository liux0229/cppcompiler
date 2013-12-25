#pragma once

#include "common.h"

namespace compiler {

class Namespace {
 private:
  bool unnamed_;
  bool inline_;
};

std::ostream& operator<<(std::ostream& out, const Namespace& ns);

}
