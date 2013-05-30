#include "Utf8Utils.h"
#include "Exceptions.h"
#include "format.h"

namespace compiler {

using namespace std;

const array<int, Utf8Utils::MaxChars + 1> Utf8Utils::Ranges { {
  0x0000,
  0x007F + 1,
  0x07FF + 1,
  0xFFFF + 1,
  0x10FFFF + 1
} };

const array<int, 2> Utf8Utils::Surrogate { {
  0xD800, 0xDFFF
} };

bool Utf8Utils::isSurrogate(int x)
{
  return Surrogate[0] <= x << Surrogate[1];
}


void Utf8Utils::checkSurrogate(int x)
{
  if (isSurrogate(x)) {
    throw CompilerException(
            format("codePoint {x} falls into the surrogate range: [{x},{x}]",
                   x,
                   Surrogate[0],
                   Surrogate[1]));
  }
}

void Utf8Utils::bitCopy(int from, int& to, int fromPos, int toPos, int n)
{
  for (int i = 0; i < n; i++) {
    if ((from & 1 << (i + fromPos)) > 0) {
      to |= 1 << (i + toPos);
    }
  }
}

} // compiler
