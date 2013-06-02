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

  static int hexToInt(int c);

  static bool isIdentifierNonDigit(int c);

  static bool isIdentifierStart(int c);

  static bool isWhiteSpaceNoNewLine(int c);

  static bool isWhiteSpace(int c);

};

} // compiler
