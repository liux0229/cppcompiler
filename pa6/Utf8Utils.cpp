#include "Utf8Utils.h"
#include "Exceptions.h"
#include "format.h"
#include "common.h"
#include "Utf8Encoder.h"
#include <cctype>
#include <map>
#include <algorithm>

namespace compiler {

using namespace std;

// See C++ standard 2.11 Identifiers and Appendix/Annex E.1
const vector<pair<int, int>> AnnexE1 =
{
	{0xA8,0xA8},
	{0xAA,0xAA},
	{0xAD,0xAD},
	{0xAF,0xAF},
	{0xB2,0xB5},
	{0xB7,0xBA},
	{0xBC,0xBE},
	{0xC0,0xD6},
	{0xD8,0xF6},
	{0xF8,0xFF},
	{0x100,0x167F},
	{0x1681,0x180D},
	{0x180F,0x1FFF},
	{0x200B,0x200D},
	{0x202A,0x202E},
	{0x203F,0x2040},
	{0x2054,0x2054},
	{0x2060,0x206F},
	{0x2070,0x218F},
	{0x2460,0x24FF},
	{0x2776,0x2793},
	{0x2C00,0x2DFF},
	{0x2E80,0x2FFF},
	{0x3004,0x3007},
	{0x3021,0x302F},
	{0x3031,0x303F},
	{0x3040,0xD7FF},
	{0xF900,0xFD3D},
	{0xFD40,0xFDCF},
	{0xFDF0,0xFE44},
	{0xFE47,0xFFFD},
	{0x10000,0x1FFFD},
	{0x20000,0x2FFFD},
	{0x30000,0x3FFFD},
	{0x40000,0x4FFFD},
	{0x50000,0x5FFFD},
	{0x60000,0x6FFFD},
	{0x70000,0x7FFFD},
	{0x80000,0x8FFFD},
	{0x90000,0x9FFFD},
	{0xA0000,0xAFFFD},
	{0xB0000,0xBFFFD},
	{0xC0000,0xCFFFD},
	{0xD0000,0xDFFFD},
	{0xE0000,0xEFFFD}
};

// See C++ standard 2.11 Identifiers and Appendix/Annex E.2
const vector<pair<int, int>> AnnexE2
{
	{0x300,0x36F},
	{0x1DC0,0x1DFF},
	{0x20D0,0x20FF},
	{0xFE20,0xFE2F}
};

namespace {
  bool inSortedRange(int c, const vector<pair<int, int>>& ranges) {
    auto it = upper_bound(
                ranges.begin(),
                ranges.end(),
                make_pair(c, -1), 
                [](const pair<int, int>& a, const pair<int, int>& b) {
                  return a.first < b.first;
                });
    if (it == ranges.begin()) {
      return false;
    }
    --it;
    return c <= it->second;
  }

  bool isNonDigit(int c) {
    // cannot call isalpha if c is not within [0,255]
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') ||
           c == '_';
  }
}

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

bool Utf8Utils::isValidUnicode(int x)
{
  return (x >= 0 && x < Surrogate[0]) ||
         (x > Surrogate[1] && x <= Ranges.back());
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

int Utf8Utils::hexToInt(int c)
{
  if (isdigit(c)) {
    return c - '0';
  } else if (isupper(c)) {
    return c - 'A' + 10;
  } else if (islower(c)) {
    return c - 'a' + 10;
  } else {
    Throw("Character [{x}] is not a hex char");
    return -1;
  }
}

bool Utf8Utils::isIdentifierNonDigit(int c) {
  return isNonDigit(c) || inSortedRange(c, AnnexE1);
}

bool Utf8Utils::isIdentifierStart(int c) {
  return isIdentifierNonDigit(c) && !inSortedRange(c, AnnexE2);
}

bool Utf8Utils::isWhiteSpaceNoNewLine(int c) {
  return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

bool Utf8Utils::isWhiteSpace(int c) {
  return isWhiteSpaceNoNewLine(c) || c == '\n';
}

int Utf8Utils::getSimpleEscaped(int x)
{
  static const map<int, int> simples = {
    { 'n', '\n' },
    { 't', '\t' },
    { 'v', '\v' }, 
    { 'b', '\b' },
    { 'r', '\r' },
    { 'f', '\f' },
    { 'a', '\a' },
    { '\\', '\\' },
    { '?', '?' },
    { '\'', '\'' },
    { '"', '"' }
  };
  auto it = simples.find(x);
  if (it == simples.end()) {
    Throw("Bad simple escape sequence \\{}", static_cast<char>(x));
  }
  return it->second;
}

int Utf8Utils::getEscapedOctal(
                    vector<int>::const_iterator begin,
                    vector<int>::const_iterator end)
{
  int s = 0;
  for (auto it = begin; it != end; ++it) {
    CHECK(*it >= '0' && *it <= '7');
    s = s * 8 + (*it - '0');
  }
  return s;
}

int Utf8Utils::getEscapedHex(
                  vector<int>::const_iterator begin,
                  vector<int>::const_iterator end)
{
  int64_t s = 0;
  for (auto it = begin; it != end; ++it) {
    s = s * 16 + Utf8Utils::hexToInt(*it);
    if (s > numeric_limits<int>::max()) {
      Throw("Escaped character too large: >= {} from \\x{}",
            s,
            Utf8Encoder::encode(vector<int>(begin, end)));
    }
  }
  return s;
}

int Utf8Utils::getEscapedCodePoint(
                 vector<int>::const_iterator& begin,
                 vector<int>::const_iterator end) {
  int codePoint;
  if (*begin == '\\') {
    if (*(begin + 1) != 'x') {
      if (isdigit(*(begin + 1))) {
        CHECK(begin + 4 <= end);
        codePoint = getEscapedOctal(begin + 1, begin + 4);
        begin += 4;
      } else {
        codePoint = getSimpleEscaped(*(begin + 1));
        begin += 2;
      }
    } else {
      auto it = begin;
      for (it += 2; it < end && isxdigit(*it); ++it) {
      }
      codePoint = getEscapedHex(begin + 2, it);
      begin = it;
    }
  } else {
    codePoint = *begin;
    ++begin;
  }
  return codePoint;
}

// match if str ends with `dChar)`
bool Utf8Utils::dCharMatch(std::vector<int>::const_iterator strStart, 
                           std::vector<int>::const_iterator strEnd, 
                           const std::vector<int>& dChar) {
  int na = strEnd - strStart;
  int nb = dChar.size();
  CHECK(na >= 1 + nb);
  if (*(strEnd - 1 - nb) == ')') {
    for (int i = 0; i < nb; ++i) {
      if (*(strEnd - 1 - i) != dChar[nb - 1 - i]) {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

} // compiler
