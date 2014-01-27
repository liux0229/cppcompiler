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
  const char* what() const noexcept(true) override
  {
    return message_.c_str();
  }
private:
  std::string message_;
};
