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
#include <functional>

#define CHECK(f) assert(f)
#define MCHECK(f, msg) do {\
  if (!(f)) {\
  std::cerr << (msg) << std::endl;\
  assert(f);\
  }\
} while (false)

#define CALL_MEM_FUNC(obj, ptr) ((obj).*ptr)

#define MakeUnique(type) using U ## type = std::unique_ptr<type>
#define MakeShared(type) using S ## type = std::shared_ptr<type>

namespace compiler {

  // Platform specific definitions
  
  // The purpose of Char16_t and Char32_t definitions is to enable us
  // to "use" char16_t and char32_t as distinct types since VC++
  // define them as aliases to integers
  // See below for type traits specializations for them.

  struct Char16_t {
    Char16_t() = default;
    Char16_t(char16_t value) : value_(value) {}
    operator char16_t() const { return value_; }
    char16_t value_;
  };

  struct Char32_t {
    Char32_t() = default;
    Char32_t(char32_t value) : value_(value) {}
    operator char32_t() const { return value_; }
    char32_t value_;
  };

  template<typename Obj, typename R, typename ...Args>
  class Delegate {
  public:
    using Func = R(Obj::*)(Args...);
    Delegate(Obj* obj, Func func) : obj_(obj), func_(func) { }
    R operator()(Args... args) {
      return (obj_->*func_)(std::forward<Args>(args)...);
    }
  private:
    Obj* obj_;
    Func func_;
  };

  // verify by doing this I am forcing Args to also be able to take reference
  template<typename Obj, typename R, typename ...Args>
  auto make_delegate(R(Obj::*func)(Args...), Obj* obj) ->
    Delegate<Obj, R, Args...> {
    return Delegate<Obj, R, Args...>(obj, func);
  }

  class ScopeGuard {
  public:
    ScopeGuard(std::function<void()> action)
      : action_(action) {
    }
    ~ScopeGuard() {
      action_();
    }

  private:
    std::function<void()> action_;
  };

#if MSVC
  using std::make_unique;
  using int32_t = int;
#else
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
#endif

  template<typename... Args>
  void Throw(const char* fmt, Args&&... args)
  {
    throw CompilerException(format(fmt, std::forward<Args>(args)...));
  }

  inline bool hasCommandlineSwitch(std::vector<std::string>& args,
                                   const char* name)
  {
    auto it = std::find(args.begin(), args.end(), name);
    bool ret = false;
    if (it != args.end()) {
      ret = true;
      args.erase(it);
    }
    return ret;
  }

  namespace {

    // EndOfFile: synthetic "character" to represent the end of source file
    const int EndOfFile = -1;

  } // anonymous

} // compiler

namespace std {

  template<> 
  struct is_integral<compiler::Char16_t> : integral_constant<bool, true> { 
  };

  template<>
  struct is_integral<compiler::Char32_t> : integral_constant<bool, true> {
  };

  template<>
  struct is_signed<compiler::Char16_t> : integral_constant<bool, false> {
  };

  template<>
  struct is_signed<compiler::Char32_t> : integral_constant<bool, false> {
  };

} // std
