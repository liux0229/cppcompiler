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
      } else if (*it == '+' || *it == '-') {
        ++it;
      }
      exp = true;
    } else {
      Throw("Unexpected character {}:({x})", char(*it), char(*it));
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
    cerr << format("ERROR: {} while parsing {}\n", 
                   e.what(),
                   Utf8Encoder::encode(vector<int>(start, end)));
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
    writer_.emit_user_defined_literal_floating(
      token.dataStrU8(),
      udSuffix,
      Utf8Encoder::encode(vector<int>(start, end)));
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
  
  string input = token.dataStrU8();
  if (type == FT_FLOAT) {
    float x = PA2Decode_float(input);
    writer_.emit_literal(input, type, &x, sizeof(x));
  } else if (type == FT_DOUBLE) {
    double x = PA2Decode_double(input);
    writer_.emit_literal(input, type, &x, sizeof(x));
  } else {
    long double x = PA2Decode_long_double(input);
    // cout << "x=" << x << '\n';
    writer_.emit_literal(input, type, &x, sizeof(x));
  }

  return true;
} 

bool FloatLiteralPostTokenizer::put(const PPToken& token)
{
  // to simplify parsing, require ud-suffix to start with '_'
  auto it = find(token.data.begin(), token.data.end(), '_');
  if (it != token.data.end()) {
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
