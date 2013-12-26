// TODO: it is possible to use a separate compilation model
// after we have a clear understanding of the interfaces between
// the sub-modules

#include "SemanticParser.h"

#include <memory>
#include <vector>
#include <functional>
#include <sstream>
#include <map>

#define EXB(func) #func, make_delegate(&Base::func, static_cast<Base*>(this))

#include "parsers/Base-inl.cpp"
#include "parsers/SimpleDeclaration-inl.cpp"
#include "parsers/ConstantExpression-inl.cpp"

#define EX(func) #func, make_delegate(&ParserImp::func, this)

namespace compiler {

using namespace std;

namespace SemanticParserImp {

class ParserImp : 
        SimpleDeclaration,
        ConstantExpression
{
public:
  ParserImp(const vector<UToken>& tokens, const ParserOption& option)
    : Base(tokens, option) {
  }
  void process() {
    TR(EX(translationUnit));
  }

private:

  void translationUnit() {
    while (!isEof() && BT(true, EXB(simpleDeclaration))) {
    }
    if (!isEof()) {
      BAD_EXPECT("<eof>");
    }
  }

};

}

void SemanticParser::process()
{
  SemanticParserImp::ParserImp(tokens_, option_).process();
}

}

#undef EX
