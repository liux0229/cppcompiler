#pragma once

#include <ostream>
#include <string>

namespace compiler {

class Utf8Encoder
{
public:
  static std::ostream& encode(std::ostream& oss, int x);
  static std::string encode(int x);
};

} // compiler
