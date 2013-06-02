#pragma once

#include "Decoder.h"
#include <functional>
#include <algorithm>

namespace compiler {

class TrigraphDecoder : public Decoder {
public:
  void put(int c) override;
private:
  int n_ { 0 };
};

}
