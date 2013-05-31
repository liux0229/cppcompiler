#include "UniversalCharNameDecoder.h"
#include "common.h"
#include <limits>

namespace compiler {

using namespace std;

void UniversalCharNameDecoder::put(int c)
{
  switch (n_) {
    case 0:
      if (c == '\\') {
        ++n_;
      } else {
        send_(c);
      }
      break;
    case 1:
      if (c == 'u' || c == 'U') {
        ++n_;
        type_ = c;
      }  else {
        n_ = 0;
        send_('\\');
      }
      break;
    default:
      if (!isxdigit(c)) {
        send_('\\');
        send_(type_);
        for (int i = 0; i < n_ - 2; ++i) {
          send_(ch_[i]);
        }
        send_(c);
        n_ = 0;
      } else {
        ch_[n_++ - 2] = c;
        int desired = type_ == 'u' ? 4 : 8;
        int m = n_ - 2;
        if (m == desired) {
          int64_t sum = 0;
          for (int i = 0; i < m; ++i) {
            sum = sum * 16 + Utf8Utils::hexToInt(ch_[i]);
          }
          checkChar(sum);
          send_(sum);
          n_ = 0;
        }
      }
      break;
  }
}

void UniversalCharNameDecoder::checkChar(int64_t x)
{
  if (x < 0 || x >= numeric_limits<int32_t>::max()) {
    Throw("Universal char {x} out of range", x);
  }
  Utf8Utils::checkSurrogate(static_cast<int>(x));
  // TODO: check char outside of string
}

}
