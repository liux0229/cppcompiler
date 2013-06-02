#pragma once

// Implemented according to: http://tools.ietf.org/html/rfc3629

#include "Utf8Utils.h"
#include "Decoder.h"
#include <string>
#include <functional>

namespace compiler {

class Utf8Decoder : public Decoder
{
public:
	void put(int c) override;
private:
  int getCodePoint() const;
  void validate() const;

  int nchars_ {0};
  int buf_[Utf8Utils::MaxChars];
  int desiredChars_ {0};
};

} // compiler
