#include "FloatLiteralPostTokenizer.h"
#include "Utf8Encoder.h"
#include "common.h"
#include "PostTokenUtils.h"
#include <algorithm>
#include <vector>

namespace compiler {

using namespace std;

namespace {

typedef vector<int>::const_iterator It;

void floatLiteralInternal(It start, It end)
{
  bool fraction = false;
  bool exp = false;
  bool digitAfterExp;
  bool digit = false;
  for (auto it = start; it < end; ++it) {
    if (isdigit(*it)) {
      // extending with a digit is always ok
      if (exp) {
        digitAfterExp = true;
      } else {
        digit = true;
      }
    } else if (*it == '.') {
      if (exp || fraction) {
        // after E no decimal point is allowed
        Throw("Unexpected `.`. exp: {} fraction: {}", exp, fraction);
      }
      fraction = true;
    } else if (*it == 'e' || *it == 'E') {
      if (!digit || exp) {
        Throw("Unexpected {}. digit: {} exp: {}", char(*it), digit, exp);
      }
      if (it == end - 1) {
        Throw("Unexpected end of input after {}", char(*it));
      } else if (*(it + 1) == '+' || *(it + 1) == '-') {
        ++it;
      }
      exp = true;
    } else {
      Throw("Unexpected character {x}", *it);
    }
  }

  if (!digit) {
    Throw("No digit found");
  }
  if (!exp && !fraction) {
    Throw("Bad input. exp: {}, fraction: {}", exp, fraction);
  }
  if (exp && !digitAfterExp) {
    Throw("No digit after e");
  }
}

bool floatLiteral(It start, It end)
{
  try {
    floatLiteralInternal(start, end);
  } catch (const CompilerException& e) {
    // Turn on this for more details of float parsing
    // cerr << format("ERROR: {} while parsing {}\n", 
    //                e.what(),
    //                Utf8Encoder::encode(vector<int>(start, end)));
    return false;
  }

  return true;
}

}

bool FloatLiteralPostTokenizer::handleUserDefined(
        const PPToken& token,
        It start,
        It end,
        const string& udSuffix) {
  if (floatLiteral(start, end)) {
    receiver_.put(*GetPostTokenLiteral::get(
                  token.dataStrU8(),
                  FT_DOUBLE,
                  Utf8Encoder::encode(vector<int>(start, end)),
                  udSuffix));
    return true;
  } else {
    return false;
  }
}

bool FloatLiteralPostTokenizer::handleFloat(const PPToken& token)
{
  EFundamentalType type = FT_DOUBLE;
  It end = token.data.end();
  int suffix = token.data.back();
  if (suffix == 'F' || suffix == 'f') {
    type = FT_FLOAT;
    end = token.data.end() - 1;
  } else if (suffix == 'L' || suffix == 'l') {
    type = FT_LONG_DOUBLE;
    end = token.data.end() - 1;
  }
  if (!floatLiteral(token.data.begin(), end)) {
    return false;
  }
  
  using GetPostTokenLiteral::get;
  string input = token.dataStrU8();
  if (type == FT_FLOAT) {
    receiver_.put(*get(input, type, PA2Decode_float(input)));
  } else if (type == FT_DOUBLE) {
    receiver_.put(*get(input, type, PA2Decode_double(input)));
  } else {
    receiver_.put(*get(input, type, PA2Decode_long_double(input)));
  }

  return true;
} 

bool FloatLiteralPostTokenizer::put(const PPToken& token)
{
  // to simplify parsing, require ud-suffix to start with '_'
  auto it = find(token.data.begin(), token.data.end(), '_');
  if (it != token.data.end()) {
    // make sure the suffix does not contain '+' or '-'
    if (find(it, token.data.end(), '+') != token.data.end() ||
        find(it, token.data.end(), '-') != token.data.end()) {
      cerr << format("bad ud_suffix for {}", token.dataStrU8()) << endl;
      return false;
    }
    return handleUserDefined(
              token,
              token.data.begin(), 
              it,
              Utf8Encoder::encode(vector<int>(it, token.data.end())));
  } else {
    return handleFloat(token);
  }
}

}
