#pragma once

#include <functional>
#include <algorithm>

namespace compiler {

class TrigraphDecoder {
public:
  void sendTo(std::function<void (int)> send) {
    send_ = send;
  }
  void put(int c);
private:
  std::function<void (int)> send_;
  int n_ { 0 };
};

}
