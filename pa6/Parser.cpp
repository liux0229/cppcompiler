// TODO: 
// 1. use multiple inheritance for composition
// circular reference is a problem
// 2. What happens if I use contexpr instead of static const inside a function
// 4. We could build some simple format of error reporting
//    1) we could report the failure happens when try to parse a certain NT
//    2) Display the line and the relevant position
//       ERROR: expect OP_RPAREN; got: simple sizeof KW_SIZEOF
#include "Parser.h"
#include <memory>
#include <vector>
#include <functional>
#include <sstream>

namespace compiler {

using namespace std;

// TODO: this function can be optimized by using a single ostringstream
// as the "environment"
string ASTNode::toStr(string indent, bool collapse) const {
  const char* indentInc = "  ";
  ostringstream oss;
  if (isTerminal) {
    oss << indent;
    if (type != ASTType::Terminal) {
      oss << getASTTypeName(type) << ": ";
    }
    oss << token->toSimpleStr();
  } else {
    // if this node has a single child and collapse is enabled,
    // do not print the enclosing node and don't indent
    if (collapse && children.size() == 1) {
      oss << children.front()->toStr(indent, collapse);
    } else {
      oss << indent << getASTTypeName(type) << ":";
      for (auto& child : children) {
        oss << "\n" << child->toStr(indent + indentInc, collapse);
      }
    }
  }
  return oss.str();
}

class ParserImp
{
public:
  ParserImp(const vector<UToken>& tokens)
    : tokens_(tokens) { }
  AST process() {
    AST root = constantExpression();
    cout << root->toStr(true /* collapse */) << endl;
    return root;
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
    return get(ASTType::ConditionalExpression, move(c));
  }

  void qmark(vector<AST>& c) {
    c.push_back(getAdv());
    c.push_back(expression());
    c.push_back(expect(OP_COLON));
    c.push_back(assignmentExpression());
  }

  AST logicalOrExpression() {
    return conditionalRepeat(ASTType::LogicalAndExpression, 
                             &ParserImp::logicalAndExpression, 
                             OP_LOR);
  }

  AST logicalAndExpression() {
    return conditionalRepeat(ASTType::LogicalAndExpression, 
                             &ParserImp::inclusiveOrExpression, 
                             OP_LAND);
  }

  AST expression() {
    return conditionalRepeat(ASTType::Expression,
                             &ParserImp::assignmentExpression, 
                             OP_COMMA);
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
    return get(ASTType::AssignmentExpression, move(c));
  }

  AST assignmentOperator() {
    if (isSimple({
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
        })) {
      return getAdv(ASTType::AssignmentOperator); 
    }
    complainExpect("assignment op");
    return nullptr;
  }

  AST inclusiveOrExpression() {
    return conditionalRepeat(ASTType::InclusiveOrExpression, 
                             &ParserImp::exclusiveOrExpression, 
                             OP_BOR);
  }

  AST exclusiveOrExpression() {
    return conditionalRepeat(ASTType::ExclusiveOrExpression, 
                             &ParserImp::andExpression, 
                             OP_XOR);
  }

  AST andExpression() {
    return conditionalRepeat(ASTType::AndExpression, 
                             &ParserImp::equalityExpression, 
                             OP_AMP);
  }

  AST equalityExpression() {
    return conditionalRepeat(
              ASTType::EqualityExpression,
              &ParserImp::relationalExpression, 
              ASTType::EqualityOperator,
              { OP_EQ, OP_NE });
  }

  AST relationalExpression() {
    return conditionalRepeat(
              ASTType::RelationalExpression,
              &ParserImp::shiftExpression,
              ASTType::RelationalOperator,
              { OP_LT, OP_GT, OP_LE, OP_GE });
  }

  AST shiftExpression() {
    return conditionalRepeat(
              ASTType::ShiftExpression,
              &ParserImp::additiveExpression,
              bind(&ParserImp::shiftOperator, this)); 
  }

  AST shiftOperator() {
    if (isSimple(OP_LSHIFT)) {
      return getAdv(ASTType::ShiftOperator);
    } else if (isSimple(OP_RSHIFT_1)) {
      vector<AST> c;
      c.push_back(getAdv());
      c.push_back(expect(OP_RSHIFT_2));
      return get(ASTType::ShiftOperator, move(c));
    } else {
      return nullptr;
    }
  }

  AST additiveExpression() {
    return conditionalRepeat(
              ASTType::AdditiveExpression,
              &ParserImp::mulplicativeExpression,
              ASTType::AdditiveOperator,
              { OP_PLUS, OP_MINUS });
  }

  AST mulplicativeExpression() {
    return conditionalRepeat(
              ASTType::MulplicativeExpression,
              &ParserImp::pmExpression,
              ASTType::MulplicativeOperator,
              { OP_STAR, OP_DIV, OP_MOD });
  }

  AST pmExpression() {
    return conditionalRepeat(
              ASTType::PmExpression,
              &ParserImp::castExpression,
              ASTType::PmOperator,
              { OP_DOTSTAR, OP_ARROWSTAR });
  }

  AST castExpression() {
    if (isSimple(OP_LPAREN)) {
      vector<AST> c;
      c.push_back(castOperator());
      c.push_back(castExpression());
      return get(ASTType::CastExpression, move(c));
    } else {
      return unaryExpression();
    }
  }

  AST typeIdInParen(ASTType type) {
    vector<AST> c;
    c.push_back(expect(OP_LPAREN));
    c.push_back(typeId());
    c.push_back(expect(OP_RPAREN));
    return get(type, move(c));
  }

  AST castOperator() {
    return typeIdInParen(ASTType::CastOperator);
  }

  AST unaryExpression() {
    vector<AST> c;
    if (isSimple(KW_SIZEOF)) {
      c.push_back(getAdv());
      if (isSimple(OP_LPAREN)) {
        c.push_back(typeIdInParen(ASTType::TypeIdInParen));
      } else if (isSimple(OP_DOTS)) {
        c.push_back(getAdv());
        c.push_back(expect(OP_LPAREN));
        c.push_back(expectIdentifier());
        c.push_back(expect(OP_RPAREN));
      } else {
        c.push_back(unaryExpression());
      }
      return get(ASTType::UnaryExpression, move(c));
    } if (isSimple(KW_ALIGNOF)) {
      c.push_back(getAdv());
      c.push_back(typeIdInParen(ASTType::TypeIdInParen));
      return get(ASTType::UnaryExpression, move(c));
    } else if (isUnaryOperator()) {
      c.push_back(getAdv(ASTType::UnaryOperator));
      c.push_back(unaryExpression());
      return get(ASTType::UnaryExpression, move(c));
    } else if (isSimple(KW_NOEXCEPT)) {
      return noExceptExpression();
    } else if (AST e = newOrDeleteExpression()) {
      return e;
    } else {
      return postfixExpression();
    }
  }

  AST newOrDeleteExpression() {
    if (isSimple(OP_COLON2)) {
      AST colon2 = getAdv();
      if (isSimple(KW_NEW)) {
        return newExpression(move(colon2));
      } else if (isSimple(KW_DELETE)) {
        return deleteExpression(move(colon2));
      } else {
        // TODO: these kinds of messages expose internal implementations
        complainExpect("KW_NEW or KW_DELETE");
        return nullptr;
      }
    } else if (isSimple(KW_NEW)) {
      return newExpression(nullptr);
    } else if (isSimple(KW_DELETE)) {
      return deleteExpression(nullptr);
    } else {
      return nullptr;
    }
  }

  AST postfixExpression() {
    return nullptr;
  }

  AST noExceptExpression() {
    vector<AST> c;
    c.push_back(expect(KW_NOEXCEPT));
    c.push_back(expect(OP_LPAREN));
    c.push_back(expression());
    c.push_back(expect(OP_RPAREN));
    return get(ASTType::NoExceptExpression, move(c));
  }

  bool isUnaryOperator() {
    return isSimple({
              OP_INC,
              OP_DEC,
              OP_STAR,
              OP_AMP,
              OP_PLUS,
              OP_MINUS,
              OP_LNOT,
              OP_COMPL
           });
  }

  /* =============
   *  declarators
   * =============
   */
  AST typeId() {
    return nullptr;
  }

  /* ===============
   *  new or delete
   * ===============
   */
  AST newExpression(AST colon2) {
    return nullptr;
  }

  AST deleteExpression(AST colon2) {
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
  AST get(ASTType type = ASTType::Terminal) const {
    return make_unique<ASTNode>(type, &cur());
  }
  AST getAdv(ASTType type = ASTType::Terminal) {
    auto r = get(type);
    adv();
    return r;
  }
  AST get(ASTType type, vector<AST>&& c) const {
    return make_unique<ASTNode>(type, move(c));
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
    return getAdv();
  }
  AST expectIdentifier() {
    if (!isIdentifier()) {
      complainExpect("identifier");
    }
    return getAdv(ASTType::Identifier);
  }

  bool isIdentifier() const {
    return cur().isIdentifier();
  }
  bool isSimple(ETokenType type) const {
    if (!cur().isSimple()) {
      return false;
    }
    return static_cast<const PostTokenSimple&>(cur()).type == type;
  }
  bool isSimple(const vector<ETokenType>& types) const {
    for (ETokenType type : types) {
      if (isSimple(type)) {
        return true;
      }
    }
    return false;
  }

  // Note: it is possible to use the follow set when FIRST does not match to
  // efficiently rule out the e-derivation - this might be helpful in emitting
  // useful error messages
  // @parseSep - return nullptr to indicate a non-match; throw to indicate parse
  //             failure
  AST conditionalRepeat(ASTType type, 
                        SubParser sub, 
                        function<AST ()> parseSep) {
    vector<AST> c;
    c.push_back(MEM_FUNC(*this, sub)());
    while (AST sep = parseSep()) {
      c.push_back(move(sep));
      c.push_back(MEM_FUNC(*this, sub)());
    }
    return get(type, move(c));
  }
  AST conditionalRepeat(ASTType type, 
                        SubParser sub, 
                        ASTType sepType,
                        const vector<ETokenType>& seps) {
    return conditionalRepeat(
              type,
              sub,
              [this, sepType, &seps]() -> AST {
                if (isSimple(seps)) {
                  return getAdv(sepType);  
                } else {
                  return nullptr;
                }
              });
  }
  AST conditionalRepeat(ASTType type, SubParser sub, ETokenType sep) {
    return conditionalRepeat(type, 
                             sub, 
                             ASTType::Terminal, 
                             vector<ETokenType>{sep});
  }

  const vector<UToken>& tokens_;
  size_t index_ { 0 };
};

AST Parser::process()
{
  return ParserImp(tokens_).process();
}

}
