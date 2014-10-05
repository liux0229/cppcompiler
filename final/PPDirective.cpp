#include "PPDirective.h"
#include "PPDirectiveUtil.h"
#include "CtrlExprEval.h"
#include "PostTokenReceiver.h"
#include "PostTokenizer.h"
#include "SourceReader.h"
#include "common.h"
#include <sstream>

namespace compiler {

using namespace std;

namespace {
void checkTrailing(const vector<PPToken>& directive, size_t i) {
  i = skipWhite(directive, i);
  if (i < directive.size()) {
    Throw("Trailing token found for {}: {}",
          directive[0].dataStrU8(),
          directive[i].dataStrU8());
  }
}

string postTokenizeToString(const PPToken& token,
                            const string& target) {
#if 0
  cout << format("postTokenizeToString: {} {}\n", token.dataStrU8(),
                                                  token.dataStrU8().size());
#endif
  if (!token.isStringOrUserDefinedLiteral()) {
    Throw("string literal expected for {}, {} received", 
          target,
          token.dataStrU8());
  }

  string result;
  int count = 0;
  auto receive = [&](const Token& token) {
    if (count++ > 0) {
      return;
    }
    auto& t = dynamic_cast<const TokenLiteralBase&>(token);
    if (t.isUserDefined()) {
      Throw("Use of user-defined string literal for {}: {}", 
            target,
            t.toStr());
    } else if (t.type != FT_CHAR) {
      Throw("{} is not a UTF8 string: {}",
            target,
            t.toStr());
    } else {
      auto& name = dynamic_cast<const TokenLiteral<string>&>(t);
      result = name.data;
      // post-tokenizer will produce a trailing 0
      CHECK(result.back() == 0);
      result.pop_back();
    }
  };
  TokenReceiver receiver(receive);
  Tokenizer postTokenizer(receiver, 
                              /* noStrCatForNewLine, not significant */
                              true);
  postTokenizer.put(token);
  postTokenizer.put(PPToken(PPTokenType::Eof, {}));
  
  return result;
}

} // anoymous

bool PPDirective::isEnabled(size_t level) const
{
  if (level + 1 > ifBlock_.size()) {
    return true;
  }
  int state = ifBlock_[ifBlock_.size() - 1 - level];
  // cout << format("if block state = {x}", state) << endl;
  return state & 0x1;
}

void PPDirective::checkEmpty(const char* cur) const
{
  if (ifBlock_.empty()) {
    Throw("Unexpected {}: not inside an #if block", cur);
  }
}

void PPDirective::checkElse(const char* cur) const
{
  CHECK(!ifBlock_.empty());
  if (ifBlock_.back() & 0x4) {
    Throw("Unexpected {} after #else", cur);
  }
}

bool PPDirective::evaluateIf(const vector<PPToken>& dirs)
{
  using namespace std::placeholders;
  CtrlExprEval ctrlExprEval(false /* print result */,
                            [this](const string& identifier) {
                              // what we return here is standard-undefined
                              return false;
                            });
  TokenReceiver receiver(bind(&CtrlExprEval::put, &ctrlExprEval, _1));
  Tokenizer postTokenizer(receiver, true /* noStrCatForNewLine */);

  // we special handle defined x or defined(x) before macro expansion takes
  // place
  vector<PPToken> directive;
  directive.reserve(dirs.size());
  int state = 0;
  size_t start = 0;
  string identifier;
  bool matched = false;
  for (size_t i = 1; i < dirs.size(); ++i) {
    switch (state) {
      case 0:
        if (isIdentifier(dirs[i], "defined")) {
          state = 1;
          start = directive.size();
        }
        break;
      case 1:
        if (dirs[i].isId()) {
          identifier = dirs[i].dataStrU8();
          matched = true;
        } else if (isLParen(dirs[i])) {
          state = 2;
        } else if (!dirs[i].isWhite()) {
          state = 0;
        }
        break;
      case 2:
        if (dirs[i].isId()) {
          identifier = dirs[i].dataStrU8();
          state = 3;
        } else if (!dirs[i].isWhite()) {
          state = 0;
        }
        break;
      case 3:
        if (isRParen(dirs[i])) {
          matched = true;
        } else if (!dirs[i].isWhite()) {
          state = 0;
        }
        break;
      default:
        CHECK(false);
        break;
    }
    if (matched) {
      matched = false;
      state = 0;
      CHECK(!identifier.empty());
      bool defined = macroProcessor_.isDefined(identifier);
      directive.erase(directive.begin() + start, directive.end());
      
      vector<int> data;
      if (defined) {
        data.push_back('1');
      } else {
        data.push_back('0');
      }
      directive.push_back(PPToken(PPTokenType::PPNumber, move(data)));
    } else {
      directive.push_back(dirs[i]);
    }
  }
  vector<PPToken> expanded = macroProcessor_.expand(directive);
  for (auto& token : expanded) {
    postTokenizer.put(token);
  }

  postTokenizer.put(PPToken(PPTokenType::NewLine, {}));
  UTokenLiteral result = ctrlExprEval.getResult();

  if (!result) {
    Throw("evaluate #if results in error");
  }
  // cout << "evaluate #if => " << result->toIntegralStr() << endl;

  return !result->isIntegralZero();
}

void PPDirective::handleIf(const vector<PPToken>& directive) {
  if (!isEnabled(0)) {
    // if a parent block is disabled, any child block is also disabled
    ifBlock_.push_back(0);
    return;
  }
  string dir = directive[0].dataStrU8();
  if (dir == "ifdef" || dir == "ifndef") {
    size_t i = skipWhite(directive, 1);
    if (i == directive.size() || !directive[i].isId()) {
      Throw("Expect identifider after {}", dir);
    }
    bool defined = macroProcessor_.isDefined(directive[i].dataStrU8());
    if (dir == "ifndef") {
      defined = ! defined;
    }
    checkTrailing(directive, i + 1);
    ifBlock_.push_back((static_cast<int>(defined) << 1) | static_cast<int>(defined));
  } else {
    bool r = evaluateIf(directive);
    ifBlock_.push_back((static_cast<int>(r) << 1) | static_cast<int>(r));
  }
}

void PPDirective::handleElif(const vector<PPToken>& directive)
{
  checkEmpty("#elif");
  checkElse("#elif");

  if (!isEnabled(1)) {
    return;
  }
  int& state = ifBlock_.back();
  int result = evaluateIf(directive);
  
  state = (state >> 1) << 1 | result;
  state |= result << 1;
}

void PPDirective::handleElse(const vector<PPToken>& directive)
{
  checkEmpty("#else");
  checkElse("#else");

  if (!isEnabled(1)) {
    ifBlock_.back() |= 0x4;
    return;
  }

  int& state = ifBlock_.back();
  
  // cout << format("state = {x}", state) << endl;
  state = ((state >> 1) << 1) | !(state >> 1 & 0x1);
  state |= 0x4;

  checkTrailing(directive, 1);
}

void PPDirective::handleEndif(const vector<PPToken>& directive)
{
  checkEmpty("#endif");
  ifBlock_.pop_back();
  if (isEnabled(0)) {
    checkTrailing(directive, 1);
  }
}

void PPDirective::handleInclude(const vector<PPToken>& dirs)
{
  auto expanded = macroProcessor_.expand(
                    vector<PPToken>(dirs.begin() + 1, dirs.end()));
  size_t i = skipWhite(expanded, 0);

  if (skipWhite(expanded, i + 1) < expanded.size()) {
    Throw("Trailing token after header-name");
  }

  if (i < expanded.size() && expanded[i].type == PPTokenType::HeaderName) {
    string header = expanded[i].dataStrU8();
    header = header.substr(1, header.size() - 2);
    sourceReader_->include(header);
  } else if (i < expanded.size()) {
    sourceReader_->include(postTokenizeToString(expanded[i],
                                                "header-name"));
  } else {
    Throw("header-name or string-literal expected after #include");
  }
}

void PPDirective::handleLine(const vector<PPToken>& dirs)
{
  auto expanded = macroProcessor_.expand(
                    vector<PPToken>(dirs.begin() + 1, dirs.end()));
  size_t i = skipWhite(expanded, 0);
  if (i == expanded.size() || expanded[i].type != PPTokenType::PPNumber) {
    Throw("expect pp-number after #line");
  }
  {
    int count = 0;
    auto receive = [&](const Token& token) {
      if (count++ > 0) {
        return;
      }
      if (token.getType() != TokenType::Literal) {
        Throw("expect literal after #line, got {}", token.toStr());
      }
      auto& t = dynamic_cast<const TokenLiteralBase&>(token);
      if (!t.isIntegral()) {
        Throw("expect integer after #line, got {}", token.toStr());
      }
      long long number = t.toSigned64();
      CHECK(number >= 0);
      // so the next __LINE__ will get number
      sourceReader_->line() = number - 1; 
    };
    TokenReceiver receiver(receive);
    Tokenizer postTokenizer(receiver, 
                                /* noStrCatForNewLine, not significant */
                                true);
    postTokenizer.put(expanded[i]);
    postTokenizer.put(PPToken(PPTokenType::Eof, {}));
  }
  i = skipWhite(expanded, i + 1);
  if (i < expanded.size()) {
    if (skipWhite(expanded, i + 1) < expanded.size()) {
      Throw("Trailing token after #line number file");
    }
    sourceReader_->file() = postTokenizeToString(expanded[i], "file"); 
  }
}

void PPDirective::handleError(const vector<PPToken>& dirs)
{
  ostringstream oss;
  for (size_t i = 1; i < dirs.size(); ++i) {
    oss << (dirs[i].isWhite() ? " " : dirs[i].dataStrU8());
  }
  Throw("#error:{}", oss.str());
}

void PPDirective::handlePragma(const vector<PPToken>& dirs)
{
  vector<PPToken> tokens;
  for (size_t i = 1; i < dirs.size(); ++i) {
    if (!dirs[i].isWhite()) {
      tokens.push_back(dirs[i]);
    }
  }
  if (tokens.size() != 1 || !isIdentifier(tokens[0], "once")) {
    return;
  }

  sourceReader_->pragmaOnce(dirs[0].file);
}

void PPDirective::handleDirective()
{
  size_t index = 0;
  while (index < directive_.size() && directive_[index].isWhite()) {
    ++index;
  }
  vector<PPToken> directive { directive_.begin() + index, directive_.end() };
  directive_.clear();

  if (directive.empty()) {
    // null-directive
    return;
  }
  auto& start = directive[0];
  if (start.type != PPTokenType::Identifier) {
    if (isEnabled(0)) {
      Throw("preprocessing directive does not start with an identifier");
    } else {
      return;
    }
  }
  string dir = start.dataStrU8();
  if (dir == "if" || dir == "ifdef" || dir == "ifndef") {
    handleIf(directive);
  } else if (dir == "elif" ) {
    handleElif(directive);
  } else if (dir == "else") {
    handleElse(directive);
  } else if (dir == "endif") {
    handleEndif(directive);
  } else {
    if (!isEnabled(0)) {
      // ignore this directive
      return;
    }
    if (dir == "define" || dir == "undef") {
      macroProcessor_.def(directive);
    } else if (dir == "include") {
      handleInclude(directive);
    } else if (dir == "line") {
      handleLine(directive);
    } else if (dir == "error") {
      handleError(directive);
    } else if (dir == "pragma") {
      handlePragma(directive);
    } else {
      Throw("bad preprocessing directive {}", dir);
    }
  }
}

void PPDirective::handleExpand()
{
  if (!isEnabled(0)) {
    text_.clear();
  }
  if (text_.empty()) {
    return;
  }

  vector<PPToken> expanded = macroProcessor_.expand(text_);
  text_.clear();

  for (size_t i = 0; i < expanded.size(); ++i) {
    auto& token = expanded[i];
    if (!isIdentifier(token, "_Pragma")) {
      send_(token);
    } else {
      size_t j = i + 1;
      for (; j < expanded.size() && expanded[j].isWhite(); ++j) { }
      if (j == expanded.size() || !isLParen(expanded[j])) {
        Throw("_Pragma must be followed by (");
      }
      for (++j; j < expanded.size() && expanded[j].isWhite(); ++j) { }
      if (j == expanded.size() || 
          expanded[j].type != PPTokenType::StringLiteral) {
        Throw("_Pragma( must be followed by a string-literal");
      }
      string literal = expanded[j].dataStrU8();
      for (++j; j < expanded.size() && expanded[j].isWhite(); ++j) { }
      if (j == expanded.size() || !isRParen(expanded[j])) {
        Throw("_Pragma not closed by )");
      }
      i = j;

      // assume the only pragma we support is 'once' so we don't need to
      // escape or pre-tokenize
      string type = literal.substr(literal.find('"') + 1);
      // cout << "type=" << type << endl;
      CHECK(type.back() == '"');
      type.pop_back();
      if (type == "once") {
        sourceReader_->pragmaOnce(token.file);
      }
    }
  }
}

void PPDirective::put(const PPToken& t)
{
  // TODO: optimize away this copy
  PPToken token = t;
  // track __FILE__ and __LINE__ for identifier (because only identifiers
  // have the potential of becoming __FILE__ or __LINE__)
  if (token.isId()) {
    token.line = sourceReader_->line();
    token.file = sourceReader_->file();
  }

  bool isText = true;
  if (token.type == PPTokenType::Eof) {
    if (!ifBlock_.empty()) {
      Throw("unterminated #if block");
    }
    handleExpand();
    send_(token);
    return;
  } else if (token.type == PPTokenType::NewLine) {
    if (state_ == 1) {
      handleDirective();
      isText = false;
    }
    state_ = 0;
  } else {
    switch (state_) {
      case 0:
        if (isPound(token)) {
          handleExpand();
          state_ = 1;
          isText = false;
        } else if (token.type != PPTokenType::WhitespaceSequence) {
          state_ = 2;
        } 
        // else: white-space, state_ is still 0
        break;
      case 1:
        directive_.push_back(token);
        isText = false;
        break;
      default:
        CHECK(state_ == 2);
        // collect the token into text
        break;
    }
  }
  if (isText) {
    text_.push_back(token);
  }
}
}
