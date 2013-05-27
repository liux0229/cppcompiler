#pragma once

#include "Exceptions.h"
#include <sstream>
#include <string>
#include <iomanip>
#include <limits>
#include <type_traits>
// used for debug output only
#include <iostream>

namespace compiler {

/*
 * Currently parses fmt at runtime.
 * To use '{' need to escape it.
 */

// This function is itself used to generate messages in its own exceptions
template<typename... Args>
std::string format(const char* fmt, Args&&... args);

template<typename T>
typename std::enable_if<!std::is_integral<T>::value>::type
setFormatHex(std::ostream& oss, T&& value)
{
  throw CompilerException(format("value cannot be formatted as hex: {}", 
                                 std::forward<T>(value)));
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value>::type
setFormatHex(std::ostream& oss, T&& value)
{
  int32_t x = static_cast<int32_t>(value);
  oss << std::showbase << std::hex << std::setfill('0');
  if (x <= std::numeric_limits<uint16_t>::max()) {
    oss << std::setw(4);
  } else {
    oss << std::setw(8);
  }
}

template<typename T>
void formatOutput(std::ostream& oss, char fmt, T&& value)
{
  auto f = oss.flags();
  switch (fmt) {
    case 'x':
      setFormatHex(oss, std::forward<T>(value));
      break;
    default:
      throw CompilerException(format("Unrecognized fmt character: {}", fmt));
      break;
  }
  oss << value;
  oss.flags(f);
}

std::ostream& format(std::ostream& oss, const char* fmt)
{
  const char* s = fmt;
  if (!s) {
    return oss;
  }

  while (*s) {
    if (*s == '\\' && *(s + 1) == '{') {
      // An escaped '{' character
      ++s;
    } else if (*s == '{') {
      throw CompilerException(
              format("fmt should not contain \\{. "
                     "Fewer arguments are provided. "
                     "Near '{}' inside '{}'", 
                     s,
                     fmt));
    }
    oss << *s++;
  }
  return oss;
}

template<typename T, typename... Args>
std::ostream& format(std::ostream& oss, const char* fmt, T&& value, Args&&... args)
{
  const char* s = fmt;
  if (!s) {
    throw CompilerException(
            format("null fmt with positive number of arguments: {}...",
                   std::forward<T>(value)));
  }

  while (*s) {
    if (*s == '\\' && *(s + 1) == '{') {
      // An escaped '{' character
      oss << *(s + 1);
      s += 2;
    } else if (*s == '{') {
      // try to detect the enclosing '}'
      const char* p;
      for (p = s + 1; *p && *p != '}'; ++p) { }
      if (*p != '}') {
        throw CompilerException(
                format("ill formed fmt: no enclosing }: '{}' inside '{}'", 
                       s,
                       fmt));
      }
      if (p - s > 2) {
        throw CompilerException(
                format("Too many characters inside \\{}: {}", 
                       std::string(s + 1, p - s - 1)));
      } else if (p - s == 2) {
        formatOutput(oss, *(s + 1), std::forward<T>(value));
      } else {
        oss << value;
      }
      return format(oss, p + 1, std::forward<Args>(args)...);
    } else {
      oss << *s++;
    }
  }

  // if we are still here, this indicates we have not exhausted all the arguments
  throw CompilerException(
          format("More arguments provided than needed: {}...",
                 std::forward<T>(value)));
}

template<typename... Args>
std::string format(const char* fmt, Args&&... args)
{
  std::ostringstream oss;
  oss << std::showpoint;
  format(oss, fmt, std::forward<Args>(args)...);
  return oss.str();
}

} // compiler
