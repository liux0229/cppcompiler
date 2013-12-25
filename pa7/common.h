#pragma once

#include "Exceptions.h"
#include "format.h"
#include <cassert>
#include <utility>
#include <memory>
#include <iostream> // so that clients can easily use std::cout to debug
#include <algorithm>
#include <set>
#include <map>
#include <vector>

namespace compiler {

#define CHECK(f) assert(f)
#define MCHECK(f, msg) do {\
                         if (!(f)) {\
                           std::cerr << (msg).c_str() << std::endl;\
                           assert(f);\
                         }\
                       } while (false)

#define CALL_MEM_FUNC(obj, ptr) ((obj).*ptr)

namespace {

// EndOfFile: synthetic "character" to represent the end of source file
constexpr int EndOfFile = -1;

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename... Args>
void Throw(const char* fmt, Args&&... args)
{
  throw CompilerException(format(fmt, std::forward<Args>(args)...));
}

} // anoymous

struct ParserOption
{
  static bool hasSwitch(std::vector<std::string>& args, const char* name)
  {
    auto it = std::find(args.begin(), args.end(), name);
    bool ret = false;
    if (it != args.end()) {
      ret = true;
      args.erase(it);
    }
    return ret;
  }

  bool isTrace { false };
  bool isCollapse { true };
};

} // compiler

