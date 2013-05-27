#include "format.h"
#include <iostream>
#include <string>
#include <exception>
#include <functional>
using namespace std;

using compiler::format;

void g(function<string ()> f)
{
  try {
    cout << f() << endl;
  } catch (const std::exception& e) {
    cout << "caught: " << e.what() << endl;
  }
}

int main()
{
  try {
    g([] { return format(nullptr, "hello"); });
    g([] { return format("a\\{ provided:{}", "hello"); });
    g([] { return format("test"); });
    g([] { return format("test {} test", 1234); });
    g([] { return format("test {} test {}", 1.01, std::string("hello")); });
    g([] { return format("test {} test {}", 1.01); });
    g([] { return format("test {} test {}", 1.01, 2, 3); });
    g([] { return format("test {} test {", 1.01, 2); });
    g([] { return format("test {x} test", (1 << 16) - 1); });
    g([] { return format("test {x} test", 1.0); });
    g([] { return format("test {x test", 1); });
    g([] { return format("test {x} test", "abc"); });
    g([] { return format("test {xy} test", 1); });
    g([] { return format("test {y} test", 1); });
    g([] { return format("test {x} {} {x} test", (1 << 18) - 1, 234, 55); });
  } catch (const std::exception& e) {
    cout << "caught: " << e.what() << endl;
  }
}
