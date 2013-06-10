#include "common.h"
#include "CtrlExprEval.h"
#include <functional>
#include <unordered_map>

namespace compiler {

using namespace std;

namespace {

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

UTokenLiteral operator*(const UTokenLiteral& lhs, const UTokenLiteral& rhs) {
  if (!lhs->isSigned() || !rhs->isSigned()) {
    return getToken(FT_UNSIGNED_LONG_LONG_INT, 
                    lhs->toUnsigned64() * rhs->toUnsigned64());
  } else {
    return getToken(FT_LONG_LONG_INT, 
                    lhs->toSigned64() * rhs->toSigned64());
  }
}

UTokenLiteral operator/(const UTokenLiteral& lhs, const UTokenLiteral& rhs) {
  if (rhs->isIntegralZero()) {
    Throw("0 as divisor when evaluating {}/{}", 
          lhs->toIntegralStr(),
          rhs->toIntegralStr());
  }
  if (!lhs->isSigned() || !rhs->isSigned()) {
    return getToken(FT_UNSIGNED_LONG_LONG_INT, 
                    lhs->toUnsigned64() / rhs->toUnsigned64());
  } else {
    return getToken(FT_LONG_LONG_INT, 
                    lhs->toSigned64() / rhs->toSigned64());
  }
}

UTokenLiteral operator%(const UTokenLiteral& lhs, const UTokenLiteral& rhs) {
  if (rhs->isIntegralZero()) {
    Throw("0 as divisor when evaluating {}%{}", 
          lhs->toIntegralStr(),
          rhs->toIntegralStr());
  }
  // cout << format("{}/{}", lhs->toIntegralStr(), rhs->toIntegralStr()) 
  //      << endl;
  if (!lhs->isSigned() || !rhs->isSigned()) {
    return getToken(FT_UNSIGNED_LONG_LONG_INT, 
                    lhs->toUnsigned64() % rhs->toUnsigned64());
  } else {
    return getToken(FT_LONG_LONG_INT, 
                    lhs->toSigned64() % rhs->toSigned64());
  }
}

const map<ETokenType, 
          function<UTokenLiteral (const UTokenLiteral&,
                                  const UTokenLiteral&)>
         > binaryOps = {
  { OP_STAR, operator* },
  { OP_DIV,  operator/ },
  { OP_MOD,  operator% }
};

UTokenLiteral apply(ETokenType type, const UTokenLiteral& token)
{
  auto it = unaryOps.find(type);
  CHECK(it != unaryOps.end());
  return it->second(token);
}

UTokenLiteral apply(ETokenType type, 
             const UTokenLiteral& lhs, 
             const UTokenLiteral& rhs)
{
  auto it = binaryOps.find(type);
  CHECK(it != binaryOps.end());
  return it->second(lhs, rhs);
}

}

namespace ControlExpression {

struct Node;
typedef unique_ptr<Node> UNode;

struct Node 
{
  UToken eval() {
    return nullptr;
  }
  UToken token;
  UNode left;
  UNode right;
};


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
    return cur().getType() == PostTokenType::Identifier;
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

  template<typename T>
  UTokenLiteral getIntegerConstant(T value) const {
    // don't preserve the source for now
    return make_unique<PostTokenLiteral<T>>(
              "", 
              FT_LONG_LONG_INT, 
              (long long)value, 
              "");
  }

  UTokenLiteral primary() {
    if (isOp(OP_LPAREN)) {
      next();
      UTokenLiteral root = control();
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
      return getIntegerConstant(PA3Mock_IsDefinedIdentifier(*target));
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

  UTokenLiteral unary() {
    ETokenType op;
    if (isOp(OP_PLUS)  ||
        isOp(OP_MINUS) ||
        isOp(OP_LNOT)  ||
        isOp(OP_COMPL)) {
      op = getOp();
      next();
      UTokenLiteral t = primary();
      return apply(op, t);
    } else {
      return primary();
    }
  }

  UTokenLiteral mul() {
    UTokenLiteral lhs = unary();

    while (isOp(OP_STAR)  ||
           isOp(OP_DIV) ||
           isOp(OP_MOD)) {
      ETokenType op = getOp();
      next();
      UTokenLiteral rhs = unary();
      lhs = apply(op, lhs, rhs);
    }

    return lhs;
  }

  UTokenLiteral control() {
    // check left over tokens
    return mul();
  }

  UTokenLiteral parse() {
    current_ = 0;
    return control();
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
            make_unique<PostTokenIdentifier>(
              TokenTypeToStringMap.at(sToken.type)));
        return;
      }
    }
    tokens_.push_back(token.copy());
  }
}

}
