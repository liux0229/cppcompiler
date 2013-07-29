// TODO: 
// 1. use multiple inheritance for composition
// circular reference is a problem
// 2. What happens if I use contexpr instead of static const inside a function
// 4. We could build some simple format of error reporting
//    1) we could report the failure happens when try to parse a certain NT
//    2) Display the line and the relevant position
//       ERROR: expect OP_RPAREN; got: simple sizeof KW_SIZEOF
// 5. complete id-expression
#include "Parser.h"
#include <memory>
#include <vector>
#include <functional>
#include <sstream>

#define LOG() cout << format("[{}] index={} [{}]\n", \
                             __FUNCTION__, index_, cur().toStr());

#define expect(type) expectFromFunc(type, __FUNCTION__)
#define BAD_EXPECT(msg) complainExpect(msg, __FUNCTION__)

// traced call
#define TR(func) tracedCall(&ParserImp::func, #func)
#define TRF(func) ([this]() -> AST \
                   { return tracedCall(&ParserImp::func, #func); })

namespace compiler {

using namespace std;

// TODO: this function can be optimized by using a single ostringstream
// as the "environment"
string ASTNode::toStr(string indent, bool collapse) const {
  const char* indentInc = "|  ";
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
  ParserImp(const vector<UToken>& tokens, bool isTrace)
    : tokens_(tokens),
      isTrace_(isTrace) { }
  AST process() {
    AST root = TR(constantExpression);
    cout << root->toStr(true /* collapse */) << endl;
    // root = constantExpression();
    // cout << root->toStr(false /* collapse */) << endl;
    return root;
  }

private:
  typedef vector<AST> VAST;

  /* =====================
   *  constant expression
   * =====================
   */
  AST constantExpression() {
    return TR(conditionalExpression);
  }

  AST conditionalExpression() {
    VAST c;
    c.push_back(TR(logicalOrExpression));
    return finishConditionalExpression(c);
  }

  AST finishConditionalExpression(VAST& c) {
    if (isSimple(OP_QMARK)) {
      c.push_back(expect(OP_QMARK));
      c.push_back(TR(expression));
      c.push_back(expect(OP_COLON));
      c.push_back(TR(assignmentExpression));
    }
    return get(ASTType::ConditionalExpression, move(c));
  }

  AST logicalOrExpression() {
    return conditionalRepeat(ASTType::LogicalOrExpression, 
                             TRF(logicalAndExpression), 
                             OP_LOR);
  }

  AST logicalAndExpression() {
    return conditionalRepeat(ASTType::LogicalAndExpression, 
                             TRF(inclusiveOrExpression), 
                             OP_LAND);
  }

  AST expression() {
    return conditionalRepeat(ASTType::Expression,
                             TRF(assignmentExpression), 
                             OP_COMMA);
  }

  AST assignmentExpression() {
    if (isSimple(KW_THROW)) {
      return TR(throwExpression);
    }
    VAST c;
    c.push_back(TR(logicalOrExpression));
    if (isAssignmentOperator()) {
      c.push_back(getAdv(ASTType::AssignmentOperator));
      c.push_back(TR(initializerClause));
      return get(ASTType::AssignmentExpression, move(c));
    } else {
      // the assignmentExpression is swallowed, which is fine
      return finishConditionalExpression(c);
    }
  }

  bool isAssignmentOperator() const {
    return isSimple({
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
           });
  }

  AST inclusiveOrExpression() {
    return conditionalRepeat(ASTType::InclusiveOrExpression, 
                             TRF(exclusiveOrExpression), 
                             OP_BOR);
  }

  AST exclusiveOrExpression() {
    return conditionalRepeat(ASTType::ExclusiveOrExpression, 
                             TRF(andExpression), 
                             OP_XOR);
  }

  AST andExpression() {
    return conditionalRepeat(ASTType::AndExpression, 
                             TRF(equalityExpression), 
                             OP_AMP);
  }

  AST equalityExpression() {
    return conditionalRepeat(
              ASTType::EqualityExpression,
              TRF(relationalExpression), 
              ASTType::EqualityOperator,
              { OP_EQ, OP_NE });
  }

  AST relationalExpression() {
    return conditionalRepeat(
              ASTType::RelationalExpression,
              TRF(shiftExpression),
              ASTType::RelationalOperator,
              { OP_LT, OP_GT, OP_LE, OP_GE });
  }

  AST shiftExpression() {
    return conditionalRepeat(
              ASTType::ShiftExpression,
              TRF(additiveExpression),
              TRF(shiftOperator));
  }

  AST shiftOperator() {
    if (isSimple(OP_LSHIFT)) {
      return getAdv(ASTType::ShiftOperator);
    } else if (isSimple(OP_RSHIFT_1)) {
      VAST c;
      c.push_back(getAdv());
      c.push_back(expect(OP_RSHIFT_2));
      return get(ASTType::ShiftOperator, move(c));
    } else {
      // this will make tracing less regular, but it's fine
      return nullptr;
    }
  }

  AST additiveExpression() {
    return conditionalRepeat(
              ASTType::AdditiveExpression,
              TRF(mulplicativeExpression),
              ASTType::AdditiveOperator,
              { OP_PLUS, OP_MINUS });
  }

  AST mulplicativeExpression() {
    return conditionalRepeat(
              ASTType::MulplicativeExpression,
              TRF(pmExpression),
              ASTType::MulplicativeOperator,
              { OP_STAR, OP_DIV, OP_MOD });
  }

  AST pmExpression() {
    return conditionalRepeat(
              ASTType::PmExpression,
              TRF(castExpression),
              ASTType::PmOperator,
              { OP_DOTSTAR, OP_ARROWSTAR });
  }

  AST castExpression() {
    if (isSimple(OP_LPAREN)) {
      VAST c;
      AST castOp = TR(castOperator);
      if (castOp) {
        c.push_back(move(castOp));
        c.push_back(TR(castExpression));
        return get(ASTType::CastExpression, move(c));
      } // else failed to parse castOperator
        // try to parse unaryExpression instead
    }

    return TR(unaryExpression);
  }

  AST typeIdInParen(ASTType type) {
    VAST c;
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(typeId));
    c.push_back(expect(OP_RPAREN));
    return get(type, move(c));
  }

  AST castOperator() {
    size_t pos = curPos();
    try {
      return typeIdInParen(ASTType::CastOperator);
    } catch (const CompilerException&) {
      // TODO: consider possible ways to optimize this
      reset(pos);
      return nullptr;
    }
  }

  AST unaryExpression() {
    VAST c;
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
        c.push_back(TR(unaryExpression));
      }
      return get(ASTType::UnaryExpression, move(c));
    } if (isSimple(KW_ALIGNOF)) {
      c.push_back(getAdv());
      c.push_back(typeIdInParen(ASTType::TypeIdInParen));
      return get(ASTType::UnaryExpression, move(c));
    } else if (isUnaryOperator()) {
      c.push_back(getAdv(ASTType::UnaryOperator));
      c.push_back(TR(unaryExpression));
      return get(ASTType::UnaryExpression, move(c));
    } else if (isSimple(KW_NOEXCEPT)) {
      return TR(noExceptExpression);
    } else if (AST e = TR(newOrDeleteExpression)) {
      return e;
    } else {
      return TR(postfixExpression);
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
        BAD_EXPECT("KW_NEW or KW_DELETE");
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
    VAST c;
    c.push_back(TR(postfixRoot));
    return get(ASTType::PostfixExpression, move(c));
  }

  AST postfixRoot() {
    VAST c;
    c.push_back(TR(primaryExpression));
    return get(ASTType::PostfixRoot, move(c));
  }

  AST primaryExpression() {
    VAST c;
    if (isSimple(KW_TRUE)) {
      c.push_back(getAdv());
    } else if (isSimple(KW_FALSE)) {
      c.push_back(getAdv());
    } else if (isSimple(KW_NULLPTR)) {
      c.push_back(getAdv());
    } else if (isSimple(KW_THIS)) {
      c.push_back(getAdv());
    } else if (isLiteral()) {
      c.push_back(getAdv());
    } else if (isSimple(OP_LPAREN)) {
      c.push_back(getAdv());
      c.push_back(TR(expression));
      c.push_back(expect(OP_RPAREN));
    } else if (isSimple(OP_LSQUARE)) {
      c.push_back(TR(lambdaExpression));
    } else {
      c.push_back(TR(idExpression));
    }
    return get(ASTType::PrimaryExpression, move(c));
  }

  AST noExceptExpression() {
    VAST c;
    c.push_back(expect(KW_NOEXCEPT));
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(expression));
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
    throw CompilerException("typeId not implemented");
  }

  /* ===============
   *  id expression
   * ===============
   */
  AST idExpression() {
    return nullptr;
  }

  /* ===================
   *  lambda expression
   * ===================
   */
  AST lambdaExpression() {
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
    VAST c;
    if (isSimple(OP_LBRACE)) {
      c.push_back(TR(bracedInitList));
    } else {
      c.push_back(TR(assignmentExpression));
    }
    // normally we want to swallow initializerClause,
    // but it's clearer to keep it in the parse tree
    return get(ASTType::InitializerClause, move(c));
  }

  AST bracedInitList() {
    VAST c;
    c.push_back(expect(OP_LBRACE));
    if (!isSimple(OP_LBRACE)) {
      c.push_back(TR(initializerList));
      if (isSimple(OP_COMMA)) {
        c.push_back(getAdv());
      }
    }
    c.push_back(expect(OP_RBRACE));
    return get(ASTType::BracedInitList, move(c));
  }

  AST initializerList() {
    VAST c;
    c.push_back(TR(initializerClauseDots));
    while (isSimple(OP_COMMA)) {
      // Here we need to look ahead 2 chars
      if (nextIsSimple(OP_RBRACE)) {
        // initializerClauseDots cannot match OP_RBRACE
        // FIRST(initializerClauseDots) !-] OP_RBRACE
        // but the upper level can, so stop matching now
        break;
      }
      // TODO; here is an example chance where we can use FIRST and/or
      // FOLLOW to prune
      c.push_back(getAdv());
      c.push_back(TR(initializerClauseDots));
    }
    return get(ASTType::InitializerList, move(c));
  }

  AST initializerClauseDots() {
    VAST c;
    c.push_back(TR(initializerClause));
    if (isSimple(OP_DOTS)) {
      c.push_back(getAdv());
    }
    return get(ASTType::InitializerClauseDots, move(c));
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
  AST get(ASTType type, VAST&& c) const {
    return make_unique<ASTNode>(type, move(c));
  }

  const PostToken& cur() const {
    return *tokens_[index_];
  }
  size_t curPos() const {
    return index_;
  }
  void reset(size_t pos) { index_ = pos; }

  // We rarely need to look ahead 2 chars
  // but in certain cases we do
  const PostToken& next() const {
    CHECK(index_ + 1 < tokens_.size());
    return *tokens_[index_ + 1];
  }
  void adv() { 
    if (isTrace_) {
      for (int i = 0; i < traceDepth_; ++i) {
        cout << Trace::padding();
      }
      cout << format("=== MATCH [{}]\n", cur().toStr());
    }
    ++index_; 
  }

  void complainExpect(string&& expected, const char* func) const {
    Throw("[{}] expect {}; got: {}", 
          func,
          move(expected),
          cur().toStr());
  }

  AST expectFromFunc(ETokenType type, const char* func) {
    if (!isSimple(type)) {
      complainExpect(getSimpleTokenTypeName(type), func);
    }
    return getAdv();
  }
  AST expectIdentifier() {
    if (!isIdentifier()) {
      BAD_EXPECT("identifier");
    }
    return getAdv(ASTType::Identifier);
  }

  bool isIdentifier() const {
    return cur().isIdentifier();
  }
  bool isLiteral() const {
    return cur().isLiteral();
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

  bool nextIsSimple(ETokenType type) const {
    if (!next().isSimple()) {
      return false;
    }
    return static_cast<const PostTokenSimple&>(next()).type == type;
  }

  // Note: it is possible to use the follow set when FIRST does not match to
  // efficiently rule out the e-derivation - this might be helpful in emitting
  // useful error messages
  // @parseSep - return nullptr to indicate a non-match; throw to indicate parse
  //             failure
  AST conditionalRepeat(ASTType type, 
                        function<AST ()> subParser,
                        function<AST ()> parseSep) {
    VAST c;
    c.push_back(subParser());
    while (AST sep = parseSep()) {
      c.push_back(move(sep));
      c.push_back(subParser());
    }
    return get(type, move(c));
  }
  AST conditionalRepeat(ASTType type, 
                        function<AST ()> subParser,
                        ASTType sepType,
                        const vector<ETokenType>& seps) {
    return conditionalRepeat(
              type,
              subParser,
              [this, sepType, &seps]() -> AST {
                if (isSimple(seps)) {
                  return getAdv(sepType);  
                } else {
                  return nullptr;
                }
              });
  }
  AST conditionalRepeat(ASTType type, 
                        function<AST ()> subParser, 
                        ETokenType sep) {
    return conditionalRepeat(type, 
                             subParser, 
                             ASTType::Terminal, 
                             vector<ETokenType>{sep});
  }

  /* ===========================
   *  traced call functionality
   * ===========================
   */
  class Trace {
  public:
    static constexpr const char* padding() {
      return "  ";
    }
    Trace(bool isTrace,
          string&& name, 
          function<const PostToken& ()> curTokenGetter,
          int& traceDepth)
      : isTrace_(isTrace),
        name_(move(name)),
        curTokenGetter_(curTokenGetter),
        traceDepth_(traceDepth)  {
      if (isTrace_) {
        tracePadding();
        cout << format("--> {} [{}]\n", name_, curTokenGetter_().toStr());
        ++traceDepth_;
      } 
    }
    ~Trace() {
      if (isTrace_) {
        --traceDepth_;
        tracePadding();
        cout << format("<-- {} {} [{}]\n", 
                       name_, 
                       ok_ ? "OK" : "BAD",
                       curTokenGetter_().toStr());
      }
    }
    void success() { ok_ = true; }
  private:
    void tracePadding() const {
      for (int i = 0; i < traceDepth_; ++i) {
        cout << padding();
      }
    }

    bool isTrace_;
    string name_;
    function<const PostToken& ()> curTokenGetter_;
    int& traceDepth_;
    bool ok_ { false };
  };
  
  AST tracedCall(SubParser parser, const char* name) {
    Trace trace(isTrace_, name, bind(&ParserImp::cur, this), traceDepth_);
    AST root = CALL_MEM_FUNC(*this, parser)();
    trace.success();
    return root;
  }

  const vector<UToken>& tokens_;
  size_t index_ { 0 };

  bool isTrace_;
  int traceDepth_ { 0 }; 
};

AST Parser::process()
{
  return ParserImp(tokens_, isTrace_).process();
}

}
