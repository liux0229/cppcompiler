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
#include <utility>

namespace compiler {

#define CHECK(f) assert(f)
#define MCHECK(f, msg) do {\
                         if (!(f)) {\
                           std::cerr << (msg).c_str() << std::endl;\
                           assert(f);\
                         }\
                       } while (false)

#define CALL_MEM_FUNC(obj, ptr) ((obj).*ptr)

template<typename Obj, typename R, typename ...Args>
class Delegate {
 public:
  using Func = R (Obj::*)(Args...);
  Delegate(Obj* obj, Func func) : obj_(obj), func_(func) { }
  R operator()(Args... args) {
    return (obj_->*func_)(std::forward<Args>(args)...);
  }
 private:
  Obj* obj_; 
  Func func_;
};

template<typename Obj, typename R, typename ...Args>
auto make_delegate(R (Obj::*func)(Args...), Obj* obj) -> 
     Delegate<Obj, R, Args...> {
  return Delegate<Obj, R, Args...>(obj, func);
}

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

} // compiler

