#include "StringLiteralPostTokenizer.h"
#include "common.h"
#include "Utf8Utils.h"
#include "Utf8Encoder.h"
#include "Utf16Encoder.h"
#include <sstream>

namespace compiler {

using namespace std;

namespace {

string getEncoding(const vector<int>& data, int& start)
{
  if (data[0] == 'u' && data[1] == '8') {
    start = 2;
    return "u8";
  } else if (data[0] == 'u' && data[1] != '8') {
    start = 1;
    return "u";
  } else if (data[0] == 'U') {
    start = 1;
    return "U";
  } else if (data[0] == 'L') {
    start = 1;
    return "L";
  } else {
    start = 0;
    return "";
  }
}

EFundamentalType toType(const string& e)
{
  if (e == "" || e == "u8") {
    return FundamentalTypeOf<char>();
  } else if (e == "u") {
    return FundamentalTypeOf<Char16_t>();
  } else if (e == "U") {
    return FundamentalTypeOf<Char32_t>();
  } else if (e == "L") {
    return FundamentalTypeOf<wchar_t>();
  } else {
    CHECK(false);
    return FundamentalTypeOf<void>();
  }
}

void readString(const PPToken& token, 
                int start, 
                int quote,
                vector<int>& codePoints)
{
  auto it = token.data.begin() + start;
  CHECK(*it == quote);
  ++it;
  while (it < token.data.end() && *it != quote) {
    int code = Utf8Utils::getEscapedCodePoint(it, token.data.end());
    if (!Utf8Utils::isValidUnicode(code)) {
      Throw("{x} is not a valid unicode point from {}", 
            code,
            token.dataStrU8());
    }
    codePoints.push_back(code);
  }

  CHECK(it < token.data.end());
}

void readRawString(const PPToken& token, 
                int start, 
                int quote,
                vector<int>& codePoints)
{
  auto it = token.data.begin() + start;
  CHECK(*it == quote);
  ++it;
  vector<int> dChar;
  while (*it != '(') {
    dChar.push_back(*it++);
  }
  ++it;
  while (it < token.data.end()) {
    if (*it == '"') {
      if (Utf8Utils::dCharMatch(token.data.begin(), it, dChar)) {
        codePoints.erase(codePoints.end() - dChar.size() - 1, codePoints.end());
        break;
      }
    }
    codePoints.push_back(*it++);
  }
  CHECK(it < token.data.end());
} 

}

string StringLiteralTokenizer::getSource() const
{
  ostringstream oss;
  const char* sep = "";
  for (auto& t : tokens_) {
    oss << sep << t.dataStrU8();
    sep = " ";
  }
  return oss.str();
}

void StringLiteralTokenizer::handle()
{
  int n = static_cast<int>(tokens_.size());
  string encoding;
  string suffix;
  vector<int> codePoints;
  for (int i = 0; i < n; ++i) {
    auto& token = tokens_[i];
    int start = 0;
    string e = getEncoding(token.data, start);
    string s = getSuffix(token, quote());

    if (!encoding.empty() && !e.empty() && encoding != e) {
      Throw("Concatenating strings failed: encoding does not match: {} {}",
            encoding,
            e);
    }
    if (!suffix.empty() && !s.empty() && suffix != s) {
      Throw("Concatenating strings failed: suffix does not match: {} {}",
            suffix,
            s);
    }
    if (!e.empty()) {
      encoding = e;
    }
    if (!s.empty()) {
      suffix = s;
    }

    if (token.data[start] == 'R') {
      readRawString(token, start + 1, quote(), codePoints);
    } else {
      readString(token, start, quote(), codePoints);
    }
  }

  // append terminating '0'
  codePoints.push_back(0);

  // output according to encoding
  using GetTokenLiteral::get;
  EFundamentalType type = toType(encoding);
  if (type == FT_CHAR) {
    receiver_.put(*get(getSource(), 
                  type, 
                  Utf8Encoder::encode(codePoints),
                  suffix));
  } else if (type == FT_CHAR16_T) {
    receiver_.put(*get(getSource(), 
                  type, 
                  Utf16Encoder::encode(codePoints),
                  suffix));
  } else {
    CHECK(type == FT_CHAR32_T || type == FT_WCHAR_T);
    if (type == FT_CHAR32_T) {
      receiver_.put(*get(getSource(), 
                    type, 
                    vector<Char32_t>(codePoints.begin(), codePoints.end()), 
                    suffix));
    } else {
      receiver_.put(*get(getSource(), 
                    type, 
                    vector<wchar_t>(codePoints.begin(), codePoints.end()), 
                    suffix));
    }
    // receiver_.put(*get(getSource(), type, codePoints, suffix));
  }
}

void StringLiteralTokenizer::put(const PPToken& token)
{
  tokens_.push_back(token);
}

void StringLiteralTokenizer::terminate()
{
  if (tokens_.empty()) {
    return;
  }

  try {
    handle();
  } catch (const CompilerException& e) {
    cerr << format("ERROR: {}\n", e.what());
    receiver_.put(TokenInvalid(getSource()));
  }

  tokens_.clear();
}

}
