#pragma once

#include "common.h"

#include <ostream>
#include <string>
#include <vector>

namespace compiler {

class Utf16Encoder
{
public:
  static std::vector<Char16_t> encode(int x);
  static std::vector<Char16_t> encode(const std::vector<int>& xs);
};

} // compiler
