#pragma once

// Implemented according to: http://tools.ietf.org/html/rfc3629

#include "Utf8Utils.h"
#include <string>
#include <functional>

namespace compiler {

class Decoder
{
public:
  void sendTo(std::function<void (int)> send) {
    send_ = send;
  }
	virtual void put(int c) = 0;
protected:
  std::function<void (int)> send_;
};

} // compiler
