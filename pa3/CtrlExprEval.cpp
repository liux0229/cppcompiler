#include "common.h"
#include "CtrlExprEval.h"

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

}

namespace ControlExpression {

struct Node;
typedef unique_ptr<Node> UNode;
typedef unique_ptr<PostToken> UToken;

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

  void next() { ++current_; }

  ETokenType getOp() const {
    CHECK(cur().getType() == PostTokenType::Simple);
    return static_cast<const PostTokenSimple&>(token).type;
  }

  bool isOp(ETokenType type) const {
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
  UToken getIntegerConstant(T value) const {
    // don't preserve the source for now
    return GetPostTokenLiteral::get("", FT_LONG_LONG_INT, (long long)value);
  }

  UToken primary() {
    if (isOp(OP_LPAREN)) {
      next();
      UToken root = control();
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

  UToken unary() {
    ETokenType op;
    if (isOp(OP_PLUS)  ||
        isOp(OP_MINUS) ||
        isOp(OP_LNOT)  ||
        isOp(OP_COMPL)) {
      op = getOp();
      next();
      UToken t = primary();
    } else {
      return primary();
    }
  }

  UToken control() {
    // check left over tokens
    return unary();
  }

  UToken parse() {
    current_ = 0;
    return control();
  }

  void parseAndEvaluate() {
    try {
      UToken result = parse();
      CHECK(result->getType() == PostTokenType::Literal);
      cout << static_cast<PostTokenLiteralBase&>(*result).toIntegralStr() 
           << endl;
    } catch (const CompilerException& e) {
      cerr << format("ERROR: parse or eval error `{}`", e.what());
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
