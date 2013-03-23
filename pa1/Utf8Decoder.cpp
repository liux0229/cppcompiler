#include "Utf8Decoder.h"

bool Utf8Decoder::put(unsigned char c)
{
	result_ = static_cast<wchar_t>(c);
	return true;
}

wchar_t Utf8Decoder::get()
{
	wchar_t ret = result_;
	result_ = 0;
	return ret;
}
