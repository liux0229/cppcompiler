#include "Utf8Encoder.h"
#include "format.h"
#include "Utf8Utils.h"
#include "common.h"
#include <ostream>
#include <sstream>
#include <algorithm>
// debug only
#include <iostream>

using namespace std;

namespace compiler {

ostream& Utf8Encoder::encode(ostream& oss, int x)
{
  Utf8Utils::checkSurrogate(x);

  auto& ranges = Utf8Utils::Ranges;
  auto it = upper_bound(ranges.begin(), ranges.end(), x);

  if (it == ranges.begin()) {
    Throw("{x} too small to encode (< {x})", x, ranges.front());
  }

  if (it == ranges.end()) {
    Throw("{x} too big to encode (>= {x})", x, ranges.back());
  }

  int length = it - ranges.begin();
  if (length == 1) {
    // cout << format("1:{x}", x) << endl;
    oss << static_cast<unsigned char>(x);
  } else {
    int codes[Utf8Utils::MaxChars] {0};
    for (int i = 0; i < length; ++i) {
      codes[0] |= 1 << (7 - i);
    }
    for (int i = 1; i < length; ++i) {
      codes[i] |= 1 << 7;
    }
    for (int i = length - 1; i >= 0; --i) {
      int n = i > 0 ? 6 : 8 - (1 + length);
      Utf8Utils::bitCopy(x, codes[i], 6 * (length - 1 - i), 0, n);
    }

    // cout << length << ":";
    for (int i = 0; i < length; ++i) {
      // cout << format(" {x}", codes[i]);
      oss << static_cast<unsigned char>(codes[i]);
    }
    // cout << endl;
  }

  return oss;
}

string Utf8Encoder::encode(int x)
{
  ostringstream oss;
  encode(oss, x);
  return oss.str();
}

string Utf8Encoder::encode(const vector<int>& xs)
{
  ostringstream oss;
  for (int x : xs) {
    encode(oss, x);
  }
  return oss.str();
}

} // compiler
