#include "PreprocessingToken.h"

const std::vector<std::string> PPTokenTypes::Names { 
  { "whitespace-sequence" },
  { "new-line" },
  { "header-name" },
  { "identifier" },
  { "pp-number" },
  { "character-literal" },
  { "user-defined-character-literal" },
  { "string-literal" },
  { "user-defined-string-literal" },
  { "preprocessing-op-or-punc" },
  { "non-whitespace-character" },
  { "eof" },
  { "[not initialized]" }
};

