#pragma once

// Implemented according to: http://tools.ietf.org/html/rfc3629

#include "Utf8Utils.h"
#include <string>
#include <functional>

namespace compiler {

class Utf8Decoder
{
public:
  void sendTo(std::function<void (int)> send) {
    send_ = send;
  }
	void put(int c);
	// int get() const;
  // std::string getStr() const;
private:

  int getCodePoint() const;
  // bool isAccepted() const;
  void validate() const;

  std::function<void (int)> send_;
  int nchars_ {0};
  int buf_[Utf8Utils::MaxChars];
  int desiredChars_ {0};
};

} // compiler
