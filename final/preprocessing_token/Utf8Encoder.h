#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace compiler {

class Utf8Encoder
{
public:
  static std::ostream& encode(std::ostream& oss, int x);
  static std::string encode(int x);
  static std::string encode(const std::vector<int>& xs);
};

} // compiler
