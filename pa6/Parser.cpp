// TODO: break up >>
#include "Parser.h"

namespace compiler
{

using namespace std;

class ParserImp
{
public:
  ParserImp(const vector<UToken>& tokens)
    : tokens_(tokens) { }
  bool process() {

  }
private:
  // return AST
  // start from constant expression
  void declaration();
  // have a way to say a particular type
  void expect(PostTokenType::Eof type);

  const vector<UToken>& tokens_;
  size_t index { 0 };
};

bool Parser::process()
{
  return ParserImp(tokens_).process();
}

}
