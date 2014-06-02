#include "CharacterLiteralFSM.h"
#include "Utf8Encoder.h"
#include "common.h"
#include <string>
#include <cctype>
#include <unordered_set>

namespace {

bool isOctal(int x) {
  return x >= '0' && x <= '7';
}

}

namespace compiler {

using namespace std;

const unordered_set<int> SimpleEscape =
{
	'\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
};

}
