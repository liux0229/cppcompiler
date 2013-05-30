#include "common.h"
#include "format.h"
#include "Utf8Encoder.h"
#include <iostream>
#include <functional>

namespace compiler {

class Tokenizer
{
public:
  void sendTo(std::function<void (int)> send) {
    send_ = send;
  }
  void put(int c) {
    send_(c);
    if (c == EndOfFile) {
      std::cout << "EOF" << std::endl;
    } else {
      std::cout 
        << format("token:{} {x}", Utf8Encoder::encode(c), c) 
        << std::endl;
    }
  }
private:
  std::function<void (int)> send_;
};

}
