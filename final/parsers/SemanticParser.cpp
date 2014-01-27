// TODO: 
// 1. It is possible to use a separate compilation model
// after we have a clear understanding of the interfaces between
// the sub-modules
// 2. Need a better error handling mechanism, i.e. need to be able to
// differentiate between "parse error because of bad probe" to "parse error
// because program is ill formed" - some way of expressing "after this point, if
// we an error, that must be because the program is ill formed". Without this,
// if the program is ill formed, we will do unncessary parsing.

#include "SemanticParser.h"

#include <memory>
#include <vector>
#include <functional>
#include <sstream>
#include <map>

#define EXB(func) #func, make_delegate(&Base::func, static_cast<Base*>(this))

#include "parsers/Base-inl.cpp"
#include "parsers/Declaration-inl.cpp"
#include "parsers/SimpleDeclaration-inl.cpp"
#include "parsers/Expression-inl.cpp"

#define EX(func) #func, make_delegate(&ParserImp::func, this)

namespace compiler {

using namespace std;

namespace SemanticParserImp {

class ParserImp : 
        Declaration,
        SimpleDeclaration,
        Expressions
{
public:
  ParserImp(const vector<UToken>& tokens, const ParserOption& option)
    : Base(tokens, option) {
  }

  UTranslationUnit process() {
    TR(EX(translationUnit));
    return move(translationUnit_);
  }

private:

  void translationUnit() {
    while (!isEof()) {
      TR(EXB(declaration));
    }
#if 0
    if (!isEof()) {
      BAD_EXPECT("<eof>");
    }
#endif
  }
};

}

UTranslationUnit SemanticParser::process()
{
  return SemanticParserImp::ParserImp(tokens_, option_).process();
}

}

#undef EX
