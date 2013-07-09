#include "CharLiteralPostTokenizer.h"
#include "PostTokenUtils.h"
#include "Utf8Utils.h"
#include "Utf8Encoder.h"
#include <vector>
#include <limits>
#include <cctype>
#include <algorithm>
#include <string>
#include <memory>

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
  using GetPostTokenLiteral::get;
  string source = token.dataStrU8();
  unique_ptr<PostToken> pt;
  if (type == FT_CHAR) {
    pt = get(source, type, (char)codePoint, suffix);
  } else if (type == FT_CHAR16_T) {
    pt = get(source, type, (char16_t)codePoint, suffix);
  } else if (type == FT_CHAR32_T) {
    pt = get(source, type, (char32_t)codePoint, suffix);
  } else if (type == FT_WCHAR_T) {
    // the requirement says we should treat wchar_t as char32_t but their
    // signedness is different; here we depend on wchar_t has the same in memory
    // representation as char32_t
    pt = get(source, type, (wchar_t)codePoint, suffix);
  } else if (type == FT_INT) {
    pt = get(source, type, codePoint, suffix);
  } else {
    CHECK(false);
  }
  receiver_.put(*pt);
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
