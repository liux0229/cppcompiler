#include "Cy86Parser.h"

#include <set>

#define expect(type) expectSimpleFromFunc(type, __FUNCTION__)
#define expectIdentifier() expectIdentifierFromFunc(__FUNCTION__)
#define expectLiteral() expectLiteralFromFunc(__FUNCTION__)

namespace compiler {

using namespace std;
using namespace Cy86;

namespace {

SConstantValue negate(SConstantValue literal) {
  auto& f = dynamic_cast<FundalmentalValueBase&>(*literal.get());
  return f.negate();
}

}

class Cy86ParserImp {
 public:
  Cy86ParserImp(vector<UToken>&& tokens) : tokens_(move(tokens)) { }
  vector<UCy86Instruction> parse() {
    return program();
  }
 private:
  vector<UCy86Instruction> program() {
    vector<UCy86Instruction> ret;
    while (!isEof()) {
      ret.push_back(statement());
      expect(OP_SEMICOLON);
    }
    return ret;
  }

  UCy86Instruction statement() {
    auto lab = label();
    if (!lab.empty()) {
      auto stm = statement();
      stm->addLabel(lab);
      return stm;
    } else {
      string op = opcode();
      vector<UOperand> operands;
      while (!isSimple(OP_SEMICOLON)) {
        operands.push_back(operand());
      }
      return Cy86InstructionFactory::get(op, move(operands));
    }
  }

  string opcode() {
    return expectIdentifier();
  }

  UOperand operand() {
    auto r = reg();
    if (r) {
      return move(r);
    }
    if (isSimple(OP_LSQUARE)) {
      return memory();
    } else {
      return immediate();
    }
  }

  URegister reg() {
    auto index = index_;
    if (!isIdentifier()) {
      return nullptr;
    }
    auto name = expectIdentifier();
    set<string> names {
      "sp", "bp", 
      "x8", "x16", "x32", "x64",
      "y8", "y16", "y32", "y64",
      "z8", "z16", "z32", "z64",
      "t8", "t16", "t32", "t64"
    };
    if (names.find(name) == names.end()) {
      index_ = index;
      return nullptr;
    }
    return make_unique<Register>(name);
  }

  UOperand memory() {
    expect(OP_LSQUARE);
    auto r = reg();
    UImmediate imm;
    if (r) {
      if (isSimple(OP_PLUS) || isSimple(OP_MINUS)) {
        bool neg = isSimple(OP_MINUS);
        adv();
        auto literal = expectLiteral();
        if (neg) {
          literal = negate(literal);
        }
        imm = make_unique<Immediate>("", literal);
      }
    } else {
      imm = rawImmediate(); 
    }
    expect(OP_RSQUARE);
    return make_unique<Memory>(move(r), move(imm));
  }

  UImmediate immediate() {
    string lab;
    SConstantValue literal;
    if (isIdentifier()) {
      lab = expectIdentifier();
    } else if (isSimple(OP_LPAREN)) {
      if (isSimple(OP_MINUS)) {
        adv();
        literal = negate(expectLiteral());
      } else {
        auto imm = rawImmediate();
        lab = imm->label();
        literal = imm->literal();
      }
      expect(OP_RPAREN);
    } else {
      literal = expectLiteral();
    }
    return make_unique<Immediate>(lab, literal);
  }

  // TT_LITERAL
  // label
  // label + TT_LITERAL
  // label - TT_LITERAL
  UImmediate rawImmediate() {
    string lab;
    SConstantValue literal;
    if (isLiteral()) {
      literal = expectLiteral();
    } else {
      lab = expectIdentifier();
      if (isSimple(OP_PLUS) || isSimple(OP_MINUS)) {
        bool neg = isSimple(OP_MINUS);
        adv();
        literal = expectLiteral();
        if (neg) {
          literal = negate(literal);
        }
      };
    }
    return make_unique<Immediate>(lab, literal);
  }

  string label() {
    auto index = index_;
    if (!isIdentifier()) {
      return "";
    }
    auto ret = expectIdentifier();
    if (!isSimple(OP_COLON)) {
      index_ = index;
      return "";
    }
    adv();
    return ret;
  }

  const PostToken& cur() const {
    return *tokens_[index_];
  }

  void adv() {
    ++index_;
  }

  const PostToken& getAdv() {
    auto& r = cur();
    adv();
    return r;
  }

  bool isEof() const {
    return cur().isEof();
  }

  bool isSimple(ETokenType type) const {
    if (!cur().isSimple()) {
      return false;
    }
    return getSimpleTokenType(cur()) == type;
  }

  bool isIdentifier() const {
    return cur().isIdentifier();
  }

  bool isLiteral() const {
    return cur().isLiteral();
  }

  void complainExpect(string&& expected, const char* func) const {
    Throw("[{}] expect {}; got: {}", 
          func,
          move(expected),
          cur().toStr());
  }

  void expectSimpleFromFunc(ETokenType type, const char* func) {
    if (!isSimple(type)) {
      complainExpect(getSimpleTokenTypeName(type), func);
    }
    return adv();
  }

  string expectIdentifierFromFunc(const char* func) {
    if (!isIdentifier()) {
      complainExpect("identifier", func);
    }
    return getAdv().toSimpleStr();
  }

  // TODO: should provide two forms. 
  // A general form (ConstantValue)
  // A more specific form (FundalmentalValueBase)
  // TODO: check for user defined literal
  SConstantValue expectLiteralFromFunc(const char* func) {
    if (!isLiteral()) {
      complainExpect("literal", func);
    }
    return static_cast<const PostTokenLiteralBase&>(getAdv()).toConstantValue();
  }

  vector<UToken> tokens_;
  size_t index_ { 0 };
};

vector<UCy86Instruction> Cy86Parser::parse(vector<UToken>&& tokens) {
  return Cy86ParserImp(move(tokens)).parse();
}

}
