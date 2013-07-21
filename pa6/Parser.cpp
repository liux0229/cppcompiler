// TODO: use multiple inheritance for composition
// circular reference is a problem
#include "Parser.h"
#include <memory>
#include <vector>

namespace compiler {

using namespace std;

class ParserImp
{
public:
  ParserImp(const vector<UToken>& tokens)
    : tokens_(tokens) { }
  AST process() {
    return constantExpression();
  }

private:
  /* =====================
   *  constant expression
   * =====================
   */
  AST constantExpression() {
    return conditionalExpression();
  }

  AST conditionalExpression() {
    vector<AST> c;
    c.push_back(logicalOrExpression());
    if (isSimple(OP_QMARK)) {
      qmark(c);
    } 
    return get(move(c));
  }

  void qmark(vector<AST>& c) {
    c.push_back(getAdv());
    c.push_back(expression());
    c.push_back(expect(OP_COLON));
    c.push_back(assignmentExpression());
  }

  AST logicalOrExpression() {
    return conditionalRepeat(&ParserImp::logicalAndExpression, OP_LOR);
  }

  AST logicalAndExpression() {
    return conditionalRepeat(&ParserImp::inclusiveOrExpression, OP_LAND);
  }

  AST expression() {
    return conditionalRepeat(&ParserImp::assignmentExpression, OP_COMMA);
  }

  AST assignmentExpression() {
    if (isSimple(KW_THROW)) {
      return throwExpression();
    }
    vector<AST> c;
    c.push_back(logicalAndExpression());
    if (isSimple(OP_QMARK)) {
      qmark(c);
    } else {
      c.push_back(assignmentOperator());
      c.push_back(initializerClause());
    }
    return get(move(c));
  }

  AST inclusiveOrExpression() {
    return nullptr;
  }

  AST assignmentOperator() {
    // TODO: what happens if I use contexpr here?
    const static ETokenType ops[] = {
      OP_ASS,
      OP_STARASS,
      OP_DIVASS,
      OP_MODASS,
      OP_PLUSASS,
      OP_MINUSASS,
      OP_RSHIFTASS,
      OP_LSHIFTASS,
      OP_BANDASS,
      OP_XORASS,
      OP_BORASS
    };
    for (ETokenType type : ops) {
      if (isSimple(type)) {
        return getAdv(); 
      }
    }
    complainExpect("assignment op");
    return nullptr;
  }

  /* ====================
   *  initializer clause
   * ====================
   */
  AST initializerClause() {
    return nullptr;
  }

  /* ==================
   *  throw expression
   * ==================
   */
  AST throwExpression() {
    return nullptr;
  }

  /* ===================
   *  utility functions
   * ===================
   */
  typedef AST (ParserImp::*SubParser)();
  AST get() const {
    return make_unique<ASTNode>(&cur());
  }
  AST getAdv() {
    auto r = make_unique<ASTNode>(&cur());
    adv();
    return r;
  }
  AST get(vector<AST>&& c) const {
    return make_unique<ASTNode>(move(c));
  }

  const PostToken& cur() const {
    return *tokens_[index_];
  }
  void adv() { ++index_; }

  void complainExpect(string&& expected) const {
    Throw("expect {}; got: {}", 
          move(expected),
          cur().toStr());
  }

  AST expect(ETokenType type) {
    if (!isSimple(type)) {
      complainExpect(getSimpleTokenTypeName(type));
    }
    AST r = getAdv();
    return move(r);
  }

  bool isSimple(ETokenType type) const {
    if (!cur().isSimple()) {
      return false;
    }
    return static_cast<const PostTokenSimple&>(cur()).type == type;
  }

  AST conditionalRepeat(SubParser sub, ETokenType sep) {
    vector<AST> c;
    c.push_back(MEM_FUNC(*this, sub)());
    while (isSimple(sep)) {
      c.push_back(getAdv());
      c.push_back(MEM_FUNC(*this, sub)());
    }
    return get(move(c));
  }

  const vector<UToken>& tokens_;
  size_t index_ { 0 };
};

AST Parser::process()
{
  return ParserImp(tokens_).process();
}

}
