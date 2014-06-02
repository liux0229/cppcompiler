#include "Utf16Encoder.h"
#include "format.h"
#include "Utf8Utils.h"
#include "common.h"
#include <ostream>
#include <sstream>
#include <algorithm>

using namespace std;

namespace compiler {

namespace {
void encodeInternal(int x, vector<Char16_t>& v)
{
  if (!Utf8Utils::isValidUnicode(x)) {
    Throw("{} is not a valid unicode point", x);
  }

  int bound = 0x10000;
  if (x < bound) {
    v.push_back(static_cast<Char16_t>(x));
  } else {
    x -= bound;
    int a = 0xD800;
    int b = 0xDC00;
    a |= (x >> 10);
    b |= (x & ((1 << 10) - 1));

    v.push_back(static_cast<Char16_t>(a));
    v.push_back(static_cast<Char16_t>(b));
  }
}
}

vector<Char16_t> Utf16Encoder::encode(int x)
{
  vector<Char16_t> v;
  encodeInternal(x, v);
  return v;
}

vector<Char16_t> Utf16Encoder::encode(const vector<int>& xs)
{
  vector<Char16_t> v;
  for (int x : xs) {
    encodeInternal(x, v);
  }
  return v;
}

} // compiler
