#include "Utf8Decoder.h"
#include "Exceptions.h"
#include "format.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;
using compiler::format;

const int ranges[] = {
  0x0000,
  0x007F + 1,
  0x07FF +1,
  0xFFFF + 1,
  0x10FFFF + 1
};

const int surrogate[] = {
  0xD800, 0xDFFF
};

static int getDesiredChars(unsigned char c)
{
  int sum = 0;
  for (int i = 0; i < 8; i++) {
    if ((c & (1 << (7 - i))) == 0) {
      break;
    }
    ++sum;
  }
  if (sum == 1) {
    throw CompilerException(
            format("bad desired chars number of 1: {x}", c));
  }
  return sum == 0 ? 1 : sum;
}

static void checkSubsequent(unsigned char c)
{
  // check c starts with "10"
  bool success = (c & 1 << 7) > 0 && (c & 1 << 6) == 0;
  if (!success) {
    throw CompilerException(
            format("bad subsequent character: not starting with 10: {x}", c));
  }
}

static void bitCopy(int from, int& to, int pos, int n)
{
  for (int i = 0; i < n; i++) {
    if ((from & 1 << i) > 0) {
      to |= 1 << (i + pos);
    }
  }
}

static int verifyCodePoint(int x, int nchar)
{
  assert(nchar >= 0 && nchar <= Utf8Decoder::MaxChar);

  if (!(ranges[nchar - 1] <= x && x < ranges[nchar])) {
    throw CompilerException(
            format("codePoint {x} not in range: [{x},{x}]",
                   x,
                   ranges[nchar - 1],
                   ranges[nchar]));
  }

  if (surrogate[0] <= x && x <= surrogate[1]) {
    throw CompilerException(
            format("codePoint {x} falls into the surrogate range: [{x},{x}]",
                   x,
                   surrogate[0],
                   surrogate[1]));
  }

  return x;
}

int Utf8Decoder::getCodePoint() const
{
  if (nchar_ == 1) {
    return buf_[0];
  } else {
    int result = 0;
    int pos = 0;
    for (int i = nchar_ - 1; i >= 0; i--) {
      int n = i > 0 ? 6 : 7 - nchar_;
      bitCopy(buf_[i], result, pos, n);
      pos += n;
    }
    return result;
  }
}

bool Utf8Decoder::put(unsigned char c)
{
  cout << "put:" << hex << c << endl;

  if (accepted_) {
    accepted_ = false;
  }

  if (nchar_ >= MaxChar) {
    throw CompilerException(
            format("too many chars in buffer: {}", 
                   nchar_ + 1));
  }

  buf_[nchar_++] = c;
  if (nchar_ == 1) {
    desiredChars_ = getDesiredChars(c);
  } else {
    checkSubsequent(c);
  }

  if (desiredChars_ == nchar_) {
    // try to accept the character number
    result_ = verifyCodePoint(getCodePoint(), nchar_);
    accepted_ = true;
    nchar_ = 0;
  }

	return accepted_;
}

wchar_t Utf8Decoder::get() const
{
  if (!accepted_) {
    throw CompilerException("call get() when no character number is accepted yet");
  }
  return static_cast<wchar_t>(result_);
}

string Utf8Decoder::getStr() const
{
  int x = get();
  ostringstream oss;
  oss << "\\U" << hex << setfill('0') << setw(8) << x;
  return oss.str();
}

void Utf8Decoder::validate() const
{
  if (nchar_ > 0) {
    throw CompilerException(
            format("trailing octets: {0}", nchar_));
  }
}
