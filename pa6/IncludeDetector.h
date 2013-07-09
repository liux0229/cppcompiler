#pragma once

#include "PreprocessingToken.h"
#include "Utf8Encoder.h"

namespace compiler {

class IncludeDetector
{
public:
  void put(const PPToken& token) {
    switch (n_) {
      case -1:
        checkNewLine(token);
        break;
      case 0:
        if (token.type == PPTokenType::PPOpOrPunc &&
            ((token.data.size() == 1 && token.data[0] == '#') ||
             (token.data.size() == 2 && token.data[0] == '%' &&
                                        token.data[1] == ':'))) {
          n_ = 1;
        } else {
          checkNewLine(token);
        }
        break;
      case 1:
#if 0
        std::cout << "data=" << Utf8Encoder::encode(token.data) << '\n';
        std::cout << "match=" 
                  << (Utf8Encoder::encode(token.data) == "include") << '\n';
        std::cout << "type=" << token.typeName() << '\n';
#endif
        if (token.type == PPTokenType::Identifier &&
            Utf8Encoder::encode(token.data) == "include") {
          n_ = 2;
        } else {
          checkNewLine(token);
        }
        break;
      case 2:
        if (token.type != PPTokenType::WhitespaceSequence) {
          checkNewLine(token);
        }
        break;
      default:
        CHECK(false);
        break;
    } 
  }
  bool canMatchHeader() const {
   //  std::cout << "can match: n=" << n_ << '\n';
    return n_ == 2;
  }
private:
  void checkNewLine(const PPToken& token) {
    if (token.type == PPTokenType::NewLine) {
      n_ = 0;
    } else {
      n_ = -1;
    }
  }
  int n_ { 0 };
};

}
