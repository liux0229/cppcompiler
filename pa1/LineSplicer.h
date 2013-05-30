#pragma once

#include "common.h"
#include <functional>

namespace compiler {

class LineSplicer {
public:
  void sendTo(std::function<void (int)> send) {
    send_ = send;
  }
  void put(int c) {
    switch (n_) {
      case 0:
        if (c != '\\') {
          send_(c);
        } else {
          ++n_;
        }
        break;
      case 1:
        if (c != '\n') {
          // the first character cannot be part of a combination
          send_('\\');
          n_ = 0;
          put(c);
        } else {
          // the backslash and newline is eaten
          n_ = 0;
        }
        break;
      default:
        CHECK(false);
        break;
    }
  }
private:
  int n_ { 0 };
  std::function<void (int)> send_;
};

}
