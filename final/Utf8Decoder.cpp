#include "Utf8Decoder.h"
#include "Utf8Utils.h"
#include "Exceptions.h"
#include "format.h"
#include "common.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;
using compiler::format;

namespace compiler {

namespace ppToken {

namespace {

int getDesiredChars(int c)
{
  int sum = 0;
  for (int i = 0; i < 8; i++) {
    if ((c & (1 << (7 - i))) == 0) {
      break;
    }
    ++sum;
  }
  if (sum == 1) {
    Throw("bad desired chars number of 1: {x}", c);
  }
  return sum == 0 ? 1 : sum;
}

void checkSubsequent(int c)
{
  // check c starts with "10"
  bool success = (c & 1 << 7) > 0 && (c & 1 << 6) == 0;
  if (!success) {
    Throw("bad subsequent character: not starting with 10: {x}", c);
  }
}

int verifyCodePoint(int x, int nchars)
{
  CHECK(nchars >= 0 && nchars <= Utf8Utils::MaxChars);

  auto ranges = Utf8Utils::Ranges;
  if (!(ranges[nchars - 1] <= x && x < ranges[nchars])) {
    Throw("codePoint {x} not in range: [{x},{x}]",
          x,
          ranges[nchars - 1],
          ranges[nchars]);
  }

  Utf8Utils::checkSurrogate(x);

  return x;
}

} // anonymous

#if 0
bool Utf8Decoder::isAccepted() const
{
  // we have received desired # of chars and haven't failed
  return nchars_ == desiredChars_;
}
#endif

int Utf8Decoder::getCodePoint() const
{
  if (nchars_ == 1) {
    return buf_[0];
  } else {
    int result = 0;
    int pos = 0;
    for (int i = nchars_ - 1; i >= 0; i--) {
      int n = i > 0 ? 6 : 7 - nchars_;
      Utf8Utils::bitCopy(buf_[i], result, 0, pos, n);
      pos += n;
    }
    return result;
  }
}

void Utf8Decoder::put(int c)
{
  // cout << "put:" << hex << c << endl;
  if (c == EndOfFile) {
    if (desiredChars_ != nchars_) {
      Throw("Incomplete utf8 character at the end of file. "
            "Desired:{}, trailing:{}",
            desiredChars_,
            nchars_);
    }
    return send_(c);
  }

  if (nchars_ >= Utf8Utils::MaxChars) {
    Throw("too many chars in buffer: {}", nchars_ + 1);
  }

  buf_[nchars_++] = c;
  if (nchars_ == 1) {
    desiredChars_ = getDesiredChars(c);
  } else {
    checkSubsequent(c);
  }

  if (desiredChars_ == nchars_) {
    // try to accept the character number, and push it to future processing
    send_(verifyCodePoint(getCodePoint(), nchars_));

    nchars_ = 0;
    desiredChars_ = 0;
  }
}

#if 0
void Utf8Decoder::validate() const
{
  if (!isAccepted()) {
    Throw("not accepted yet. trailing octets: {}", nchars_);
  }
}

string Utf8Decoder::getStr() const
{
  int x = get();
  return format("{x}", x);
}
#endif

} // ppToken

} // compiler
