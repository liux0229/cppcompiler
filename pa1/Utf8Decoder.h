#pragma once

class Utf8Decoder
{
public:
	bool put(unsigned char c);
	wchar_t get();
private:
	wchar_t result_;
};
