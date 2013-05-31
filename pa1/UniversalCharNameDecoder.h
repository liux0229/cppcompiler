#pragma once

#include "Utf8Utils.h"
#include <functional>
#include <algorithm>
#include <cctype>

namespace compiler {

class UniversalCharNameDecoder {
public:
  void sendTo(std::function<void (int)> send) {
    send_ = send;
  }
  void put(int c);
private:
  void checkChar(int64_t c);

  std::function<void (int)> send_;
  int n_ { 0 };
  int type_;
  int ch_[8];
};

}
