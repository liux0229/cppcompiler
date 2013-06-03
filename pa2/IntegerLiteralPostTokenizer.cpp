#include "IntegerLiteralPostTokenizer.h"
#include "common.h"
#include "Utf8Utils.h"
#include <vector>
#include <map>
#include <limits>

namespace compiler {

using namespace std;

namespace {

typedef vector<int>::const_iterator It;

bool isInteger(It start, It end)
{
  bool oct = false;
  bool hex = false;
  if (*start == '0') {
    if (start + 1 == end) {
      return true;
    } else {
      if (*start == 'x' || *start == 'X') {
        hex = true;
        start += 2;
      } else {
        oct = true;
        start += 1;
      }
    }
  }
  while (start < end) {
    if (oct) {
      if (*start < '0' || *start > '7') {
        return false;
      }
    } else if (hex) {
      if (!isxdigit(*start)) {
        return false;
      }
    } else {
      if (!isdigit(*start)) {
        return false;
      }
    }
    ++start;
  }
  return true;
}

bool raise(uint64_t& r, uint64_t base, uint64_t s) 
{
  uint64_t m = numeric_limits<uint64_t>::max();
  if (r > m / base) {
    return false;
  }
  r *= base;
  if (r > m - s) {
    return false;
  }
  r += s;
  return true;
}

bool parseInteger(It start, It end, bool& octOrHex, uint64_t& r)
{
  bool oct = false;
  bool hex = false;
  if (*start == '0') {
    if (start + 1 == end) {
      return true;
    } else {
      if (*start == 'x' || *start == 'X') {
        hex = true;
        start += 2;
      } else {
        oct = true;
        start += 1;
      }
    }
  }
  r = 0;
  while (start < end) {
    if (oct) {
      if (*start < '0' || *start > '7') {
        return false;
      }
      if (!raise(r, 8, (*start - '0'))) {
        return false;
      }
    } else if (hex) {
      if (!isxdigit(*start)) {
        return false;
      }
      if (!raise(r, 16, Utf8Utils::hexToInt(*start))) {
        return false;
      }
    } else {
      if (!isdigit(*start)) {
        return false;
      }
      if (!raise(r, 10, (*start - '0'))) {
        return false;
      }
    }
    ++start;
  }
  octOrHex = oct || hex;
  return true;
}

uint64_t getMax(EFundamentalType type)
{
  static const map<int, uint64_t> m = {
    { FT_INT, numeric_limits<int>::max() },
    { FT_UNSIGNED_INT, numeric_limits<unsigned int>::max() },
    { FT_LONG_INT, numeric_limits<long>::max() },
    { FT_UNSIGNED_LONG_INT, numeric_limits<unsigned long>::max() },
    { FT_LONG_LONG_INT, numeric_limits<long long>::max() },
    { FT_UNSIGNED_LONG_LONG_INT, numeric_limits<unsigned long long>::max() }
  };
  auto it = m.find(type);
  CHECK(it != m.end());
  return it->second;
}

vector<EFundamentalType> getList(bool _unsigned, 
                                 bool _long, 
                                 bool _longlong,
                                 bool hex)
{
  if (!_unsigned && !_long && !_longlong) {
    if (!hex) {
      return { 
        FT_INT, 
        FT_LONG_INT, 
        FT_LONG_LONG_INT 
      };
    } else {
      return { 
        FT_INT, 
        FT_UNSIGNED_INT, 
        FT_LONG_INT, 
        FT_UNSIGNED_LONG_INT, 
        FT_LONG_LONG_INT,
        FT_UNSIGNED_LONG_LONG_INT 
      };
    }
  } else if (_unsigned && !_long && !_longlong) {
      return { 
        FT_UNSIGNED_INT, 
        FT_UNSIGNED_LONG_INT, 
        FT_UNSIGNED_LONG_LONG_INT 
      };
  } else if (!_unsigned && _long && !_longlong) {
    if (!hex) {
      return { 
        FT_LONG_INT, 
        FT_LONG_LONG_INT 
      };
    } else {
      return { 
        FT_LONG_INT, 
        FT_UNSIGNED_LONG_INT, 
        FT_LONG_LONG_INT,
        FT_UNSIGNED_LONG_LONG_INT 
      };
    }
  } else if (_unsigned && _long && !_longlong) {
      return { 
        FT_UNSIGNED_LONG_INT, 
        FT_UNSIGNED_LONG_LONG_INT 
      };
  } else if (!_unsigned && !_long && _longlong) {
    if (!hex) {
      return { 
        FT_LONG_LONG_INT 
      };
    } else {
      return { 
        FT_LONG_LONG_INT,
        FT_UNSIGNED_LONG_LONG_INT 
      };
    }
  } else if (_unsigned && !_long && _longlong) {
      return { 
        FT_UNSIGNED_LONG_LONG_INT 
      };
  } else {
    CHECK(false);
  }
}

}

bool IntegerLiteralPostTokenizer::handleUserDefined(
        const PPToken& token,
        It start,
        It end,
        const string& udSuffix) {
  if (!isInteger(start, end)) {
    return false;
  }

  writer_.emit_user_defined_literal_integer(
    token.dataStrU8(),
    udSuffix,
    Utf8Encoder::encode(vector<int>(start, end)));

  return true;
}

bool IntegerLiteralPostTokenizer::handleInteger(const PPToken& token)
{
  bool _unsigned = false;
  bool _long = false;
  bool _longlong = false;

  auto it = token.data.end() - 1;
  int count[4] { 0 }; // l, L, u, U
  while (it >= token.data.end() - 3) {
    if (*it == 'l') {
      ++count[0];
    } else if (*it == 'L') {
      ++count[1];
    } else if (*it == 'u') {
      ++count[2];
    } else if (*it == 'U') {
      ++count[3];
    } else {
      break;
    }
    --it;
  } 
  ++it;
  int n = token.data.end() - it;
  try {
    if (count[0] && count[1]) {
      Throw("Integer suffix cannot have both `l' and `L`");
    }
    count[0] += count[1];
    if (count[0] >= 3) {
      Throw("Integer suffix has too many L's"); 
    }
    if (count[2] && count[3]) {
      Throw("Integer suffix cannot have both `u' and `U`");
    }
    count[2] += count[3];
    if (count[2] >= 2) {
      Throw("Integer suffix has too many U's"); 
    }
    if (n == 3) {
      if (*(it + 1) == 'u' || *(it + 1) == 'U') {
        Throw("Bad integer suffix LUL");
      }
    }
    if (count[2]) {
      _unsigned = true;
    }
    if (count[0] == 1) {
      _long = true;
    } else if (count[2] == 2) {
      _longlong = true;
    }
  } catch (const CompilerException& e) {
    cerr << format("ERROR: {} while parsing {}\n", e.what(), token.dataStrU8());
  }

  uint64_t r{0};
  bool octOrHex{false};
  if (!parseInteger(token.data.begin(), it, octOrHex, r)) {
    return false;
  }

  vector<EFundamentalType> types 
    = getList(_unsigned, _long, _longlong, octOrHex);
  EFundamentalType type;
  for (auto t : types) {
    if (r <= getMax(t)) {
      type = t;
      break;
    }
  }

  string str = token.dataStrU8();
  switch (type) {
    case FT_INT: {
      int x = int(r);
      writer_.emit_literal(str, type, &x, sizeof(x));
      break;
    }
    case FT_UNSIGNED_INT: {
      unsigned int x = (unsigned int)r;
      writer_.emit_literal(str, type, &x, sizeof(x));
      break;
    }
    case FT_LONG_INT: {
      long x = long(r);
      writer_.emit_literal(str, type, &x, sizeof(x));
      break;
    }
    case FT_UNSIGNED_LONG_INT: {
      unsigned long x = (unsigned long)r;
      writer_.emit_literal(str, type, &x, sizeof(x));
      break;
    }
    case FT_LONG_LONG_INT: {
      long long x = (long long)r;
      writer_.emit_literal(str, type, &x, sizeof(x));
      break;
    }
    case FT_UNSIGNED_LONG_LONG_INT: {
      unsigned long long x = (unsigned long long)r;
      writer_.emit_literal(str, type, &x, sizeof(x));
      break;
    }
    default:
      CHECK(false);
      break;
  }
   
  return true;
}

bool IntegerLiteralPostTokenizer::put(const PPToken& token)
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
    return handleInteger(token);
  }
}

}
