#include "CharLiteralPostTokenizer.h"
#include "PostTokenUtils.h"
#include "Utf8Utils.h"
#include "Utf8Encoder.h"
#include <vector>
#include <limits>
#include <cctype>
#include <algorithm>
#include <string>

namespace compiler {

using namespace std;

namespace {

EFundamentalType getEncoding(const vector<int>& data, int& start)
{
  if (data[0] == 'u') {
    start = 1;
    return FundamentalTypeOf<char16_t>();
  } else if (data[0] == 'U') {
    start = 1;
    return FundamentalTypeOf<char32_t>();
  } else if (data[0] == 'L') {
    start = 1;
    return FundamentalTypeOf<wchar_t>();
  } else {
    start = 0;
    return FundamentalTypeOf<char>();
  }
}

}

void CharLiteralPostTokenizer::output(const PPToken& token, 
                                      EFundamentalType type, 
                                      int codePoint,
                                      const string& suffix)
{
  // storage for the values
  char ch;
  char16_t ch16;
  char32_t ch32;
  void *pData;
  int size;

  if (type == FT_CHAR) {
    ch = codePoint;
    pData = &ch;
    size = 1;
  } else if (type == FT_CHAR16_T) {
    ch16 = codePoint;
    pData = &ch16;
    size = 2;
  } else if (type == FT_CHAR32_T || type == FT_WCHAR_T) {
    ch32 = codePoint;
    pData = &ch32;
    size = 4;
  } else if (type == FT_INT) {
    pData = &codePoint;
    size = 4;
  } else {
    CHECK(false);
  }

  if (suffix.empty()) {
    writer_.emit_literal(token.dataStrU8(), type, pData, size);
  } else {
    writer_.emit_user_defined_literal_character(
        token.dataStrU8(), 
        suffix, 
        type, 
        pData, 
        size);
  }
}

void CharLiteralPostTokenizer::handle(const PPToken& token, 
                                      const string& suffix)
{
  // get encoding
  int start = 0; 
  EFundamentalType type = getEncoding(token.data, start);
  CHECK(token.data[start] == quote());
  ++start;

  auto next = token.data.begin() + start;
  // check empty character
  if (*next == quote()) {
    Throw("Empty character literal: {}", token.dataStrU8());
  }

  int codePoint = Utf8Utils::getEscapedCodePoint(next, token.data.end());

  // check whether codePoint falls into expected range

  if (!Utf8Utils::isValidUnicode(codePoint)) {
    Throw("{x} is not a valid unicode point from {}", 
          codePoint,
          token.dataStrU8());
  }

  if (type == FT_CHAR) {
    if (codePoint >= 128) {
      type = FT_INT;
    }
  } else if (type == FT_CHAR16_T) {
    if (codePoint >= 1 << 16) {
      Throw("{x} is too large for char16_t from {}", 
            codePoint,
            token.dataStrU8());
    }
  }

  // Check whether there are trailing characters
  CHECK(next != token.data.end());
  if (*next != quote()) {
    Throw("Multi-character literal not supported: {}", token.dataStrU8());
  }

  output(token, type, codePoint, suffix);
}

void CharLiteralPostTokenizer::put(const PPToken& token)
{
  // printToken(token);
  handle(token, getSuffix(token, quote()));
}

}
