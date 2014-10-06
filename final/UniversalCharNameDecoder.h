#pragma once

#include "Utf8Utils.h"
#include "Decoder.h"
#include <functional>
#include <algorithm>
#include <cctype>
#include <cstdint>

namespace compiler {

namespace ppToken {

class UniversalCharNameDecoder : public Decoder {
public:
  void put(int c) override;
private:
  void checkChar(int64_t c);

  int n_ { 0 };
  int type_;
  int ch_[8];
};

} // ppToken

} // compiler
