#pragma once

#include <array>

namespace compiler {

class Utf8Utils
{
public:
  constexpr static int MaxChars = 4;

  static const std::array<int, MaxChars + 1> Ranges;

  static const std::array<int, 2> Surrogate;

  static bool isSurrogate(int x);

  static void checkSurrogate(int x);

  static void bitCopy(int from, int& to, int fromPos, int toPos, int n);

};

} // compiler
