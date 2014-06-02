#pragma once

#include <exception>
#include <string>

class CompilerException : public std::exception
{
public:
  CompilerException(std::string message) 
  {
    message_.swap(message);
  }
#ifdef MSVC
  const char* what() const override
#else
  const char* what() const noexcept override
#endif
  {
    return message_.c_str();
  }
private:
  std::string message_;
};
