#pragma once

#include "Exceptions.h"
#include <cassert>
#include <utility>
#include <memory>

namespace compiler {

#define CHECK(f) assert(f)

namespace {

// EndOfFile: synthetic "character" to represent the end of source file
constexpr int EndOfFile = -1;

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>...));
}

template<typename... Args>
void Throw(const char* fmt, Args&&... args)
{
  throw CompilerException(format(fmt, std::forward<Args>(args)...));
}

} // anoymous

} // compiler

