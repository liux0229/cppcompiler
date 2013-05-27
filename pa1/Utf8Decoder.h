#pragma once

// Implemented according to: http://tools.ietf.org/html/rfc3629

#include <string>

class Utf8Decoder
{
public:
  static const int MaxChar = 4;

	bool put(unsigned char c);
	wchar_t get() const;
  std::string getStr() const;
  void validate() const;
private:

  int getCodePoint() const;

	int result_ {0};
  bool accepted_ {false};
  int nchar_ {0};
  unsigned char buf_[MaxChar];
  int desiredChars_ {0};
};
