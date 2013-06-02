#pragma once

#include <functional>
#include <algorithm>
#include "common.h"
#include "Decoder.h"

namespace compiler {

namespace {

void checkEof(int c) {
  if (c == EndOfFile) {
    Throw("Comment not terminated");
  }
}

}

class CommentDecoder : public Decoder {
public:
  bool turnOffForQuotedLiteral() const override { return true; }
  void put(int c) override {
    switch (n_) {
      case 0:
        if (c != '/') {
          send_(c);
        } else {
          ++n_;
        }
        break;
      case 1:
        if (c == '/') {
          n_ = 2;
        } else if (c == '*') {
          n_ = 3;
        } else {
          send_('/');
          send_(c);
          n_ = 0;
        }
      case 2:
        checkEof(c);
        // TODO: special handling for '\f' and '\v'
        if (c == '\n') {
          send_(' ');
          send_('\n');
          n_ = 0;
        }
        break;
      case 3:
        checkEof(c);
        if (c == '*') {
          n_ = 4;
        }
        break;
      case 4:
        checkEof(c);
        if (c == '/') {
          send_(' ');
          n_ = 0;
        } else {
          n_ = 3;
        }
        break;
      default:
        CHECK(false);
    }
  }
private:
  // 2 - //
  // 3 - /*
  // 4 - /* *
  int n_ { 0 };
};

}
