#include "common.h"
#include "CtrlExprEval.h"
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm>
#include <type_traits>

namespace compiler {

using namespace std;

namespace ControlExpression {

namespace {

typedef unsigned long long ULL;
typedef long long LL;

// mock implementation of IsDefinedIdentifier for PA3
// return true iff first code point is odd
bool PA3Mock_IsDefinedIdentifier(const string& identifier)
{
	if (identifier.empty())
		return false;
	else
		return identifier[0] % 2;
}

typedef unique_ptr<PostToken> UToken;
typedef unique_ptr<PostTokenLiteralBase> UTokenLiteral;

template<typename T>
UTokenLiteral getIntegerConstant(T value) {
  // don't preserve the source for now
  if (is_signed<T>::value) {
    return make_unique<PostTokenLiteral<T>>(
        "", 
        FT_LONG_LONG_INT, 
        (long long)value, 
        "");
  } else {
    return make_unique<PostTokenLiteral<T>>(
        "", 
        FT_UNSIGNED_LONG_LONG_INT, 
        (unsigned long long)value, 
        "");
  }
}

// Return a 0 constant to indicate type only
UTokenLiteral getZero(bool isSigned)
{
  if (isSigned) {
    return getIntegerConstant(0);
  } else {
    return getIntegerConstant(0u);
  }
}

// TODO: more type checks between T and the type specifier
template<typename T>
UTokenLiteral getToken(EFundamentalType type, T data) {
  return make_unique<PostTokenLiteral<T>>("", type, data, "");
}

UTokenLiteral toLiteral(UToken&& token) {
  CHECK(token->getType() == PostTokenType::Literal);
  return UTokenLiteral(static_cast<PostTokenLiteralBase*>(token.release()));
}

UTokenLiteral unaryPlus(const UTokenLiteral& token) {
  return toLiteral(token->copy()); 
}

UTokenLiteral unaryMinus(const UTokenLiteral& token) {
  if (token->isSigned()) {
    return getToken(token->type, -token->toSigned64());
  } else {
    return getToken(token->type, -token->toUnsigned64());
  }
}

UTokenLiteral unaryNot(const UTokenLiteral& token) {
  return getToken(FT_LONG_LONG_INT, token->isIntegralZero() ? 1 : 0);
}

UTokenLiteral unaryComp(const UTokenLiteral& token) {
  if (token->isSigned()) {
    return getToken(token->type, ~token->toSigned64());
  } else {
    return getToken(token->type, ~token->toUnsigned64());
  }
}

const map<ETokenType, 
          function<UTokenLiteral (const UTokenLiteral&)>> unaryOps = {
  { OP_PLUS, unaryPlus },
  { OP_MINUS, unaryMinus },
  { OP_LNOT, unaryNot },
  { OP_COMPL, unaryComp }
};

void checkDivisorZero(const UTokenLiteral& lhs, const UTokenLiteral& rhs)
{
  if (rhs->isIntegralZero()) {
    Throw("Divide by 0. LHS: {}, RHS: {}", 
          lhs->toIntegralStr(),
          rhs->toIntegralStr());
  }
}

#define OPERATOR(op) {\
  [](ULL a, ULL b) -> ULL { return a op b; }, \
  [](LL a, LL b) -> LL { return a op b; } }

const map<ETokenType,
          pair<pair<function<ULL (ULL, ULL)>, 
                    function<LL (LL, LL)>>,
               function<void (const UTokenLiteral&, 
                               const UTokenLiteral&)>
              >
         > promotedBinaryOps {
  { OP_STAR,  { OPERATOR(*), nullptr } },
  { OP_DIV,   { OPERATOR(/), checkDivisorZero } },
  { OP_MOD,   { OPERATOR(%), checkDivisorZero } },
  { OP_PLUS,  { OPERATOR(+), nullptr } },
  { OP_MINUS, { OPERATOR(-), nullptr } },
  { OP_AMP,  { OPERATOR(&), nullptr } },
  { OP_XOR,   { OPERATOR(^), nullptr } },
  { OP_BOR,   { OPERATOR(|), nullptr } }
};

#undef OPERATOR

UTokenLiteral promotedBinaryOp(
    const UTokenLiteral& lhs,
    const UTokenLiteral& rhs,
    const function<ULL (ULL, ULL)>& unsignedOp,
    const function<LL (LL, LL)>& signedOp,
    const function<void (const UTokenLiteral&, 
                         const UTokenLiteral&)>& checker,
    bool eval) {
  if (!eval) {
    return getZero(lhs->isSigned() && rhs->isSigned());
  }
  if (checker) {
    checker(lhs, rhs);
  }
  if (!lhs->isSigned() || !rhs->isSigned()) {
    return getToken(FT_UNSIGNED_LONG_LONG_INT,
                    unsignedOp(lhs->toUnsigned64(), rhs->toUnsigned64()));
  } else {
    return getToken(FT_LONG_LONG_INT,
                    signedOp(lhs->toSigned64(), rhs->toSigned64()));
  }
}

#define OPERATOR(op) {\
  [](ULL a, ULL b) -> LL { return (long long)(a op b); }, \
  [](LL a, LL b) -> LL { return a op b; } }

const map<ETokenType,
          pair<function<LL (ULL, ULL)>, 
               function<LL (LL, LL)>
              >
         > relOps {
  { OP_LT, OPERATOR(<)  },
  { OP_LE, OPERATOR(<=) },
  { OP_GT, OPERATOR(>)  },
  { OP_GE, OPERATOR(>=) },
  { OP_EQ, OPERATOR(==) },
  { OP_NE, OPERATOR(!=) }
};

#undef OPERATOR

UTokenLiteral relOp(
    const UTokenLiteral& lhs,
    const UTokenLiteral& rhs,
    const function<LL (ULL, ULL)>& unsignedOp,
    const function<LL (LL, LL)>& signedOp,
    bool eval) {
  if (!eval) {
    return getZero(true);
  }
  if (!lhs->isSigned() || !rhs->isSigned()) {
    return getToken(FT_LONG_LONG_INT, 
                    unsignedOp(lhs->toUnsigned64(), rhs->toUnsigned64()));
  } else {
    return getToken(FT_LONG_LONG_INT, 
                    signedOp(lhs->toSigned64(), rhs->toSigned64()));
  }
}

unsigned long long getShiftRHS(const UTokenLiteral& rhs) {
  unsigned long long r;
  if (rhs->isSigned()) {
    long long sr = rhs->toSigned64();
    if (sr < 0) {
      Throw("Trying to left/right shift by {}", sr);
    }
    r = sr;
  } else {
    r = rhs->toUnsigned64();
  }
  if (r >= 64) {
    Throw("Left/right shift RHS operand too large: {}", r);
  }
  return r;
}

UTokenLiteral leftShift(const UTokenLiteral& lhs,
                        const UTokenLiteral& rhs,
                        bool eval) {
  if (!eval) {
    return getZero(lhs->isSigned());
  }
  unsigned long long r = getShiftRHS(rhs);
  if (lhs->isSigned()) {
    return getToken(FT_LONG_LONG_INT, lhs->toSigned64() << r);
  } else {
    return getToken(FT_UNSIGNED_LONG_LONG_INT, lhs->toUnsigned64() << r);
  }
}

UTokenLiteral rightShift(const UTokenLiteral& lhs,
                         const UTokenLiteral& rhs,
                         bool eval) {
  if (!eval) {
    return getZero(lhs->isSigned());
  }
  unsigned long long r = getShiftRHS(rhs);
  if (lhs->isSigned()) {
    return getToken(FT_LONG_LONG_INT, lhs->toSigned64() >> r);
  } else {
    return getToken(FT_UNSIGNED_LONG_LONG_INT, lhs->toUnsigned64() >> r);
  }
}

const map<ETokenType, 
          function<UTokenLiteral (const UTokenLiteral&,
                                  const UTokenLiteral&,
                                  bool)>
         > binaryOps {
  { OP_LSHIFT, leftShift },
  { OP_RSHIFT, rightShift }
};

UTokenLiteral apply(ETokenType type, const UTokenLiteral& token)
{
  auto it = unaryOps.find(type);
  CHECK(it != unaryOps.end());
  return it->second(token);
}

UTokenLiteral apply(ETokenType type, 
             const UTokenLiteral& lhs, 
             const UTokenLiteral& rhs,
             bool eval)
{
  {
    auto it = promotedBinaryOps.find(type);
    if (it != promotedBinaryOps.end()) {
      auto& ops = it->second.first;
      auto& checker = it->second.second;
      return promotedBinaryOp(lhs, rhs, ops.first, ops.second, checker, eval);
    }
  }
  {
    auto it = relOps.find(type);
    if (it != relOps.end()) {
      auto& ops = it->second;
      return relOp(lhs, rhs, ops.first, ops.second, eval);
    }
  }
  {
    auto it = binaryOps.find(type);
    CHECK(it != binaryOps.end());
    return it->second(lhs, rhs, eval);
  }
}

const vector<vector<ETokenType>> binaryOpTable {
  { OP_STAR, OP_DIV, OP_MOD },
  { OP_PLUS, OP_MINUS },
  { OP_LSHIFT, OP_RSHIFT },
  { OP_LT, OP_LE, OP_GT, OP_GE },
  { OP_EQ, OP_NE },
  { OP_AMP },
  { OP_XOR },
  { OP_BOR },
};

} // anoymous

class Parser
{
public:
  Parser(vector<UToken> && tokens)
    : tokens_(move(tokens)) { }

  const PostToken& cur() const {
    if (current_ >= tokens_.size()) {
      Throw("parse index {} out of range: {}", current_, tokens_.size());
    }
    return *tokens_[current_];
  }

  void next() { 
    CHECK(current_ < tokens_.size());
    ++current_; 
  }

  ETokenType getOp() const {
    CHECK(cur().getType() == PostTokenType::Simple);
    return static_cast<const PostTokenSimple&>(cur()).type;
  }

  bool isOp(ETokenType type) const {
    if (current_ >= tokens_.size()) {
      return false;
    }
    auto& token = cur();
    if (token.getType() != PostTokenType::Simple) {
      return false;
    }
    return getOp() == type;
  }

  bool isIdentifier() const {
    return current_ < tokens_.size() &&
           cur().getType() == PostTokenType::Identifier;
  }

  const string& identifierOrKeyword() const {
    if (!isIdentifier()) {
      Throw("Expected identifier, got `{}`", cur().toStr());
    }
    return cur().source;
  }

  void expectRParen() {
    if (!isOp(OP_RPAREN)) {
      Throw("Expected `)`, got `{}`", cur().toStr());
    }
    next();
  }

  UTokenLiteral primary(bool eval) {
    if (isOp(OP_LPAREN)) {
      next();
      UTokenLiteral root = control(eval);
      expectRParen();
      return root;
    } else if (isIdentifier() && cur().source == "defined") {
      next();
      const string* target;
      if (isOp(OP_LPAREN)) {
        next();
        target = &identifierOrKeyword();
        next();
        expectRParen();
      } else {
        target = &identifierOrKeyword();
        next();
      }
      return getIntegerConstant(
                // Interesting note: bool is unsigned
                (long long)PA3Mock_IsDefinedIdentifier(*target));
    } else if (cur().getType() == PostTokenType::Literal) {
      auto& t = static_cast<const PostTokenLiteralBase&>(cur());
      if (!t.isIntegral()) {
        Throw("Expected an integral literal, got `{}`", t.toStr());
      }
      next();
      return t.promoteTo64();
    } else {
      const string& val = identifierOrKeyword();
      next();
      return getIntegerConstant(val == "true" ? 1 : 0);
    }
  }

  UTokenLiteral unary(bool eval) {
    ETokenType op;
    if (isOp(OP_PLUS)  ||
        isOp(OP_MINUS) ||
        isOp(OP_LNOT)  ||
        isOp(OP_COMPL)) {
      op = getOp();
      next();
      UTokenLiteral t = primary(eval);
      // note t would return the desired type
      return eval ? apply(op, t) : move(t); 
    } else {
      return primary(eval);
    }
  }

  UTokenLiteral getTokenLower(int index, bool eval) {
    return index == 0 ? unary(eval) : binary(index - 1, eval);
  }

  UTokenLiteral binary(int index, bool eval) {
    CHECK(index >= 0);
    UTokenLiteral lhs = getTokenLower(index, eval);

    const auto& ops = binaryOpTable[index];
    while (find_if(ops.begin(), 
                   ops.end(), 
                   [this](ETokenType t) { return isOp(t); }) 
            != ops.end()) {
      ETokenType op = getOp();
      next();
      UTokenLiteral rhs = getTokenLower(index, eval);
      lhs = apply(op, lhs, rhs, eval);
    }

    return lhs;
  }

  UTokenLiteral binary(bool eval) {
    return binary(binaryOpTable.size() - 1, eval);
  }

  UTokenLiteral logicalAnd(bool eval) {
    UTokenLiteral t = binary(eval);
    eval = eval && !t->isIntegralZero();
    while (isOp(OP_LAND)) {
      next();
      t = binary(eval);
      eval = eval && !t->isIntegralZero();
      // if eval started with false, then it does not matter what we return
      // but note the returned type is always right
      t = getIntegerConstant(eval ? 1 : 0);
    }
    return t;
  }

  UTokenLiteral logicalOr(bool eval) {
    UTokenLiteral t = logicalAnd(eval);
    eval = eval && t->isIntegralZero();
    while (isOp(OP_LOR)) {
      next();
      t = logicalAnd(eval);
      eval = eval && t->isIntegralZero();
      // if eval started with false, then it does not matter what we return
      // but note the returned type is always right
      t = getIntegerConstant(!eval ? 1 : 0);
    }
    return t;
  }

  UTokenLiteral control(bool eval) {
    UTokenLiteral cond = logicalOr(eval);
    if (isOp(OP_QMARK)) {
      next();
      UTokenLiteral lhs = control(eval && !cond->isIntegralZero());
      if (!isOp(OP_COLON)) {
        Throw("Expected `:`, got {}", cur().toStr());
      }
      next();
      UTokenLiteral rhs = control(eval && cond->isIntegralZero());

      // statically determine the type of the result
      bool isSigned = lhs->isSigned() && rhs->isSigned();

      // if eval started as false then it does not matter what we return
      UTokenLiteral ret = !cond->isIntegralZero() ? move(lhs) : move(rhs);

      // convert if type is not desired
      if (isSigned != ret->isSigned()) {
        CHECK(!isSigned);
        ret = getIntegerConstant(ret->toUnsigned64());
      }
      return ret;
    } else {
      return cond;
    }
  }

  UTokenLiteral parse() {
    current_ = 0;
    UTokenLiteral t = control(true);
    if (current_ != tokens_.size()) {
      CHECK(current_ < tokens_.size());
      Throw("Trailing tokens: {}...", tokens_[current_]->toStr());
    }
    return t;
  }

  void parseAndEvaluate() {
    try {
      UTokenLiteral result = parse();
      cout << result->toIntegralStr() << endl;
    } catch (const CompilerException& e) {
      cerr << format("ERROR: parse or eval error `{}`", e.what()) << endl;
      cout << "error" << endl;
    }
  }

private:
  vector<UToken> tokens_;
  size_t current_ { 0 };
};

} // ControlExpression

void CtrlExprEval::put(const PostToken& token)
{
  if (token.getType() == PostTokenType::NewLine) {
    if (!tokens_.empty()) {
      ControlExpression::Parser(move(tokens_)).parseAndEvaluate();
    }
    tokens_.clear();
  } else {
    // converting keywords into identifiers while we are collecting tokens
    if (token.getType() == PostTokenType::Simple) {
      auto& sToken = static_cast<const PostTokenSimple&>(token);
      if (sToken.type < TOTAL_KEYWORDS) {
        tokens_.push_back(
            make_unique<PostTokenIdentifier>(sToken.source));
        return;
      }
    }
    tokens_.push_back(token.copy());
  }
}

}
