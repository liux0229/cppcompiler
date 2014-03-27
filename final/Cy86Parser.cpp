#include "common.h"
#include "Cy86Parser.h"

#include <set>

#define expect(type) expectSimpleFromFunc(type, __FUNCTION__)
#define expectIdentifier() expectIdentifierFromFunc(__FUNCTION__)

namespace compiler {

using namespace std;
using namespace Cy86;

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
    string op = opcode();
    vector<UOperand> operands;
    while (!isSimple(OP_SEMICOLON)) {
      operands.push_back(operand());
    }
    return Cy86InstructionFactory::get(op, move(operands));
  }

  string opcode() {
    return expectIdentifier();
  }

  UOperand operand() {
    UOperand ret = reg();
    if (ret) {
      return ret;
    }
    if (isSimple(OP_LSQUARE)) {
      return memory();
    } else {
      return immediate();
    }
  }

  UOperand reg() {
    auto name = expectIdentifier();
    set<string> names {
      "sp", "bp", 
      "x8", "x16", "x32", "x64",
      "y8", "y16", "y32", "y64",
      "z8", "z16", "z32", "z64",
      "t8", "t16", "t32", "t64"
    };
    if (names.find(name) == names.end()) {
      return nullptr;
    }
    return make_unique<Register>(name);
  }

  UOperand memory() {
    return nullptr;
  }

  UOperand immediate() {
    // figure out how to handle ints and floats
    return nullptr;
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

  vector<UToken> tokens_;
  size_t index_;
};

vector<UCy86Instruction> Cy86Parser::parse(vector<UToken>&& tokens) {
  return Cy86ParserImp(move(tokens)).parse();
}

}
