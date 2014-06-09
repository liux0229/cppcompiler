#include "MacroProcessor.h"
#include "PPDirectiveUtil.h"
#include "PPTokenizer.h"
#include "PredefinedMacros.h"
#include <string>
#include <memory>
#include <sstream>
#include <algorithm>
#include <functional>

namespace compiler {

using namespace std;

namespace {

const string VA_ARG_STR{"__VA_ARGS__"};

template<typename T>
void trim(vector<T>& v, function<bool (const T&)> predicate)
{
  if (!v.empty() && predicate(v.back())) {
    v.pop_back();   
  }

  size_t i = 0;
  if (i < v.size() && predicate(v[i])) {
    ++i;
  }
  if (i > 0) {
    v.erase(v.begin(), v.begin() + i);
  }
}

PPToken mergeToken(const PPToken& a, const PPToken& b)
{
  // cout << format("merge {} with {}\n", a.dataStrU8(), b.dataStrU8());

  // run a "mini" inline pp-tokenizer
  size_t tokenCount = 0;
  PPToken merged;
  auto receiver = [&](const PPToken& token) {
    if (token.type != PPTokenType::Eof &&
        token.type != PPTokenType::NewLine) {
      // cout << "received token " << token.dataStrU8() << endl;
      ++tokenCount;
      merged = PPToken(token, a.file, a.line);
    }
  };

  PPTokenizer ppTokenizer;
  ppTokenizer.sendTo(receiver);

  auto run = [&](const string& s) {
    for (char c : s)
    {
      ppTokenizer.process(static_cast<unsigned char>(c));
    }
  };
  run(a.dataStrU8());
  run(b.dataStrU8());
  ppTokenizer.process(EndOfFile);

  // cout << "tokenCount = " << tokenCount << endl;
  if (tokenCount != 1) {
    // merge failed for some reason
    return a;
  } else {
    return merged;
  }
}

PPToken getLiteral(vector<MacroProcessor::TextToken>&& tokens) {
  trim<MacroProcessor::TextToken>(
      tokens, 
      [](const MacroProcessor::TextToken& t) {
          return t.token.isWhite() || t.token.isNewLine();
      });

  vector<int> data;
  bool prevSpace = false;
  data.push_back('"');

  for (auto& tt : tokens) {
    auto& t = tt.token;
    // cout << format("<{}>", t.isWhite() ? " " : t.dataStrU8());
    if (t.isWhite() || t.type == PPTokenType::NewLine) {
      if (prevSpace) {
        continue;
      }
      data.push_back(' ');
      prevSpace = true;
    } else {
      prevSpace = false;
      for (int x : t.data) {
        if (t.isQuotedOrUserDefinedLiteral() &&
            (x == '\\' || x == '"')) {
          data.push_back('\\');
        }
        data.push_back(x);
      }
    }
  }
  // cout << "\n";

  data.push_back('"');
  return PPToken(PPTokenType::StringLiteral, data);
}

bool included(const vector<string>& names, const PPToken& token) {
  if (!token.isId()) {
    return false;
  }
  return find(names.begin(), names.end(), token.dataStrU8()) != names.end();
}

vector<string> add(const vector<string>& names, const string& name) {
  vector<string> r(names);
  r.push_back(name);
  return r;
}

vector<string> add(const vector<string>& a, const vector<string>& b) {
  vector<string> r(a);
  r.insert(r.end(), b.begin(), b.end());
  return r;
}

}

bool MacroProcessor::TextToken::canExpand() {
  // cout << format("canExpand: {}=>{}\n", parentMacros, token.dataStrU8());
  if (!expand) {
    return false;
  }
  return expand = !included(parentMacros, token);
}

string MacroProcessor::Repl::toStr() const {
  if (!isParam()) {
    return token.isWhite() ? " " : token.dataStrU8();
  } else {
    ostringstream oss;
    if (parameter == -2) {
      oss << "<... ";
    } else {
      oss << format("<{}:{} ", parameter, token.dataStrU8());
    }

    if (inplaceExpand) {
      oss << "I";
    }
    if (literal) {
      oss << "L";
    }
    oss << ">";
    return oss.str();
  }
}

string MacroProcessor::Macro::toStr() const {
  ostringstream oss;
  oss << format("[{}] ", type == Type::Function ? "function" : "object");
  if (type == Type::Function) {
    oss << "(";
    const char* sep = "";
    for (auto& p : paramList) {
      oss << sep << p;
      sep = " ";
    }
    oss << ") ";
  }
  if (varArg) {
    oss << "var_arg ";
  }
  oss << "=> ";
  for (auto& repl : bodyList) {
    oss << "[";
    for (auto& r : repl) {
      oss << r.toStr();
    }
    oss << "]";
  } 
  return oss.str();
}

MacroProcessor::TextList
MacroProcessor::Macro::getReplTextList(
                    const PPToken& token,
                    const vector<string>& parentMacros,
                    const PredefinedMacros& predefinedMacros) const {
  TextList result;
  if (predefined) {
    string name = token.dataStrU8();
    PPToken replaced;
    if (name == "__LINE__") {
      CHECK(token.line >= 1);
      replaced = PPToken(PPTokenType::PPNumber,
                         toVector(format("{}", token.line)));
    } else if (name == "__FILE__") {
      CHECK(!token.file.empty());
      replaced = PPToken(PPTokenType::StringLiteral,
                         stringify(token.file));
    } else {
      replaced = predefinedMacros.get(token.dataStrU8());
    }
    result.push_back(vector<TextToken>{ 
                        TextToken(replaced, parentMacros)
                     });
  } else {
    for (auto& repl : bodyList) {
      vector<TextToken> text;
      for (auto& r : repl) {
        // annotate line and file info
        text.push_back(
            TextToken(PPToken(r.token, token.file, token.line), 
                      parentMacros));
      }
      result.push_back(move(text));
    }
  }
  return result;
}

MacroProcessor::MacroProcessor(const PredefinedMacros& predefinedMacros)
  : predefinedMacros_(predefinedMacros)
{
  Macro predefined(Macro::Type::Object, {}, false, {}, {}, true);
  macros_.insert(make_pair("__CPPGM__", predefined));
  macros_.insert(make_pair("__cplusplus", predefined));
  macros_.insert(make_pair("__STDC_HOSTED__", predefined));
  macros_.insert(make_pair("__CPPGM_AUTHOR__", predefined));
  macros_.insert(make_pair("__DATE__", predefined));
  macros_.insert(make_pair("__TIME__", predefined));
  macros_.insert(make_pair("__FILE__", predefined));
  macros_.insert(make_pair("__LINE__", predefined));
}

size_t MacroProcessor::parseParam(const vector<PPToken>& tokens, 
                                  size_t i, 
                                  vector<string>* paramList,
                                  bool* varArg) {
  // true - expect id or ...
  // false - expect , or )
  bool expectId = true;
  bool expectEnd = false;
  bool start = true;
  for (; i < tokens.size(); ++i) {
    auto& t = tokens[i];
    if (t.isWhite()) {
      continue;
    }
    if (expectId) {
      if (t.isId()) {
        // check for the same parameter
        string name = t.dataStrU8();
        if (find(paramList->begin(), paramList->end(), name) != 
            paramList->end()) {
          Throw("parameter {} appears more than once in the parameter list",
                name);
        }
        paramList->push_back(name);
      } else if (isEllipse(t)) {
        *varArg = true;
        expectEnd = true;
      } else if (start && isRParen(t)) {
        break;
      } else {
        Throw("expect identifier or ..., '{}' received", t.dataStrU8());
      }
    } else {
      if (isRParen(t)) {
        break;
      } else {
        if (expectEnd) {
          Throw("expect ')', '{}' received", t.dataStrU8());
        } else if (!isComma(t)) {
          Throw("expect ',' or ')', '{}' received", t.dataStrU8());
        }
      }
    }

    start = false;
    expectId = !expectId;
  }

  if (i == tokens.size()) {
    Throw("Parameter list does not end with ')'");
  } 
  return i + 1;
}

MacroProcessor::Repl
MacroProcessor::mapParameter(const PPToken& token, 
                             const vector<string>& paramList,
                             bool varArg) {
  int param = -1;
  if (token.isId()) {
    string name = token.dataStrU8();
    auto it = find(paramList.begin(), paramList.end(), name);
    if (it != paramList.end()) {
      param = it - paramList.begin(); 
    } else if (name == VA_ARG_STR) {
      if (!varArg) {
        Throw("use {} in replacement list of a macro which does not"
              " have the '...' parameter",
              VA_ARG_STR);
      }
      param = -2;
    }
  }
  return Repl(token, param);
}

vector<MacroProcessor::Repl>
MacroProcessor::parseRepl(vector<Repl>&& repl) {
  // replace '# param' with the internal representation
  vector<Repl> result;
  for (size_t i = 0; i < repl.size(); ++i) {
    auto& r = repl[i];
    if (!r.isParam() && isPound(r.token)) {
      size_t j;
      for (j = i + 1; j < repl.size() && repl[j].token.isWhite(); ++j) { }
      if (j == repl.size() ||
          !repl[j].isParam()) {
        Throw("# must be followed by a parameter in the replacement list");
      }
      result.push_back(move(repl[j]));
      result.back().inplaceExpand = false;
      result.back().literal = true;
      i = j;
    } else {
      result.push_back(move(r));
    }
  }
  return result;
}

void MacroProcessor::define(const vector<PPToken>& tokens)
{
  size_t i = skipWhite(tokens, 1);
  if (i == tokens.size() || !tokens[i].isId()) {
    Throw("#define must be followed by an identifier");
  }
  string name = tokens[i++].dataStrU8();

  if (name == VA_ARG_STR) {
    Throw("{} cannot be macro name", VA_ARG_STR);
  }

  Macro::Type type;
  vector<string> paramList;
  bool varArg { false };
  if (i < tokens.size() && isLParen(tokens[i])) {
    type = Macro::Type::Function;
    i = parseParam(tokens, i + 1, &paramList, &varArg);
    i = skipWhite(tokens, i);
  } else {
    if (i < tokens.size()) {
      if (!tokens[i].isWhite()) {
        Throw("Object like macro must have whitespaces between the name and"
              " the replacement list. '{}' received",
              tokens[i].dataStrU8());
      }
      ++i;
    }
    type = Macro::Type::Object;
  }

  vector<PPToken> bodyOriginal;
  CHECK(i == tokens.size() || !tokens[i].isWhite());
  for (; i < tokens.size(); ++i) {
    bodyOriginal.push_back(tokens[i]);
  }
  while (!bodyOriginal.empty() && bodyOriginal.back().isWhite()) {
    bodyOriginal.pop_back();
  }
  
  // check body starts or ends with ##
  if (!bodyOriginal.empty()) {
    if (isDoublePound(bodyOriginal.front())) {
      Throw("replacement list cannot start with ##");
    } else if (isDoublePound(bodyOriginal.back())) {
      Throw("replacement list cannot end with ##");
    }
  }
  vector<vector<Repl>> body;
  vector<Repl> repl;
  for (auto& t : bodyOriginal) {
    if (isDoublePound(t)) {
      body.push_back(
          type == Macro::Type::Function ?
            parseRepl(move(repl)) :
            move(repl));
      repl.clear();
    } else {
      repl.push_back(mapParameter(t, paramList, varArg));
    }
  }
  if (!repl.empty()) {
    body.push_back(
        type == Macro::Type::Function ?
          parseRepl(move(repl)) :
          move(repl));
  }

  // get rid of the leading and trailing spaces from each repl
  for (auto& repl : body) {
    trim<Repl>(repl, [](const Repl& t) { return t.token.isWhite(); });
  }

  if (body.size() > 1) {
    for (size_t i = 0; i < body.size(); ++i) {
      auto& repl = body[i];
      if (i > 0) {
        if (!repl.empty()) {
          repl.front().inplaceExpand = false;
        }
      }
      if (i + 1 < body.size()) {
        if (!repl.empty()) {
          repl.back().inplaceExpand = false;
        }
      }
    }
  }

  Macro macro{type, move(paramList), varArg, move(body), move(bodyOriginal)};
  auto it = macros_.find(name);
  if (it != macros_.end()) {
    if (it->second != macro) {
      Throw("macro redefined: {}", name);
    }
  } else {
    macros_.insert(make_pair(name, move(macro)));
    // cout << format("define <{}> to be {}\n", name, macros_[name].toStr());
  }
}

void MacroProcessor::undefine(const vector<PPToken>& tokens)
{
  string name;
  for (size_t i = 1; i < tokens.size(); ++i) {
    if (!tokens[i].isWhite()) {
      if (!name.empty()) {
        Throw("Trailing tokens after #undef {}: {}", 
              name,
              tokens[i].dataStrU8());
      } else if (!tokens[i].isId()) {
        Throw("Expect identifier after #undef, {} received",
              tokens[i].dataStrU8());
      } else {
        name = tokens[i].dataStrU8();
      }
    }
  }
  if (name.empty()) {
    Throw("Missing identifier after #undef");
  }
  // cout << format("macro {} undefined\n", name);
  macros_.erase(name);
}

void MacroProcessor::def(const vector<PPToken>& tokens)
{
  string directive = tokens[0].dataStrU8();
  if (directive == "define") {
    define(tokens);
  } else if (directive == "undef") {
    undefine(tokens);
  } else {
    CHECK(false);
  }
}

vector<MacroProcessor::TextToken>
MacroProcessor::merge(TextList&& textList)
{
  if (textList.empty()) {
    return {};
  }

  vector<TextToken> result;
  result.insert(result.end(), textList[0].begin(), textList[0].end());

  for (size_t i = 1; i < textList.size(); ++i) {
    auto& text = textList[i];
    if (text.empty()) {
      continue;
    }
    if (result.empty()) {
      result.insert(result.end(), text.begin(), text.end());
    } else {
      PPToken merged = mergeToken(result.back().token, text[0].token);
      result.back() = TextToken(merged, 
                                add(result.back().parentMacros,
                                    text[0].parentMacros));

      result.insert(result.end(), text.begin() + 1, text.end());
    }
  }
  return result;
}

MacroProcessor::TextList
MacroProcessor::applyFunction(const PPToken& root,
                              const Macro& macro,
                              vector<vector<TextToken>>&& args,
                              const vector<string>& parentMacros) {

  size_t nparam = macro.paramList.size();

  if (nparam == 1 && args.empty()) {
    // consider the invocation to have one empty argument
    args.push_back({});
  }
  if (!macro.varArg) {
    if (nparam != args.size()) {
      Throw("wrong number of args for macro invocation for {}; "
            "{} expected, {} received",
            root.dataStrU8(),
            macro.paramList.size(),
            args.size());
    }
  } else {
    if (nparam > args.size()) {
      Throw("wrong number of args for macro invocation for {}; "
            ">= {} expected, {} received",
            root.dataStrU8(),
            macro.paramList.size(),
            args.size());
    }
    if (args.size() > nparam) {
      size_t last = nparam;
      for (size_t i = last + 1; i < args.size(); ++i) {
        // note:
        // TextToken textToken(PPToken(PPTokenType::PPOpOrPunc, {','}), {});
        // would crash VC++2013.
        PPToken ppToken(PPTokenType::PPOpOrPunc, {','});
        args[last].push_back(TextToken(ppToken, {}));
        args[last].insert(args[last].end(), args[i].begin(), args[i].end());
      }
      args.erase(args.begin() + last + 1, args.end());
    }
  }

  TextList result;
  for (auto& repl : macro.bodyList) {
    vector<TextToken> text;
    for (auto& r : repl) {
      if (r.isParam()) {
        int param = r.parameter;
        if (param == -2) {
          param = static_cast<int>(nparam);
        }
        auto& arg = args[param];
        vector<TextToken> argText;

        if (r.literal) {
          // note the compiler crashing comment above.
          auto ppToken = getLiteral(move(arg));
          argText.push_back(TextToken(ppToken, {}));
        } else {
          for (auto& t : arg) {
            // cout << format("extract param {}: {}\n", param, t.dataStrU8());
            // TODO: we can move 't' here
            argText.push_back(t);
          }
        }

        if (r.inplaceExpand) {
          replace(argText);
        }

        // now substitue the argument
        for (auto& t : argText) {
          // t.parentMacros tracks the parent macros in the inplace replacement
          // note the compiler crashing comment above.
          PPToken ppToken(t.token, root.file, root.line);
          text.push_back(
            TextToken(ppToken, add(t.parentMacros, parentMacros)));
        }
      } else {
        PPToken ppToken(r.token, root.file, root.line);
        text.push_back(
            TextToken(ppToken, parentMacros));
      }
    } 
    result.push_back(move(text));
  }

  return result;
}

namespace {
void debug(const vector<MacroProcessor::TextToken>& text)
{
  cout << "replace:";
  for (auto& t : text) {
    cout << format("<{}>", 
                   t.token.isWhite() ? " " : t.token.dataStrU8());
  }
  cout << "\n";
}
}

// TODO: optimize names (mutate in place)
// taking a TextToken vector is necessary, consider the case ggg(a b c d)
// where ggg was produced by ##
vector<MacroProcessor::TextToken>&
MacroProcessor::replace(vector<TextToken>& text)
{
  // debug(text);

  bool expanded;
  do {
    expanded = false;
    for (auto it = text.begin(); it != text.end(); ++it) {
      const auto& t = it->token;
      if (!it->canExpand() || !t.isId()) {
        continue;
      }
      // cout << format("expanding {}({})\n", t.dataStrU8(), it->parentMacros);
      string name = t.dataStrU8();
      auto itMacro = macros_.find(name);
      if (itMacro == macros_.end()) {
        continue;
      }
      auto& macro = itMacro->second;
      if (macro.isObject()) {
        auto result = merge(macro.getReplTextList(
                              t,
                              add(it->parentMacros, name),
                              predefinedMacros_));
        it = text.erase(it);
        text.insert(it, result.begin(), result.end());
        expanded = true;
        break;
      } else {
        auto itEnd = it + 1;
        while (itEnd != text.end() && 
               (itEnd->token.isWhite() || itEnd->token.isNewLine())) {
          ++itEnd;
        }
        if (itEnd != text.end() && isLParen(itEnd->token)) {
          // find matching ')' and collect arguments
          int count = 1;
          vector<vector<TextToken>> args(1);
          for (++itEnd; itEnd != text.end(); ++itEnd) {
            if (isComma(itEnd->token) && count == 1) {
              args.push_back({});
            } else {
              if (!isRParen(itEnd->token) || count > 1) {
                // keep the parent macros for this argument token
                args.back().push_back(*itEnd);
              }
            }

            if (isRParen(itEnd->token)) {
              --count;
              if (count == 0) {
                break;
              }
            } else if (isLParen(itEnd->token)) {
              ++count;
            }
          }
          if (count > 0) {
            Throw("Cannot locate matching ')' for macro invocation for {}",
                  name);
          }
          CHECK(isRParen(itEnd->token));
          ++itEnd;
          if (args.size() == 1 && args[0].empty()) {
            args.clear();
          }

          // trim the leading and trailing whitespaces of each arg
          // (not including the ones corresponding to ...)
          for (size_t i = 0; i < args.size(); ++i) {
            if (!(macro.varArg && i >= macro.paramList.size())) {
              trim<TextToken>(args[i], [](const TextToken& t) {
                return t.token.isWhite() || t.token.isNewLine();
              });
            }
          }

          auto result = merge(applyFunction(t, 
                                            macro, 
                                            move(args), 
                                            add(it->parentMacros, name)));
#if 0
          cout << "argument: ";
          for (auto& r : result) {
            cout << format("<{}({})> ", r.token.dataStrU8(), r.parentMacros);
          }
          cout << "\n";
#endif
          it = text.erase(it, itEnd);
          text.insert(it, result.begin(), result.end());
          expanded = true;
          break;
        }
      }
    }
  } while (expanded);

  // cout << "<replace finished>" << endl;
  return text;
}

vector<PPToken> MacroProcessor::expand(const vector<PPToken>& rawText)
{
  vector<TextToken> text;
  for (auto& t : rawText) {
    if (t.isId() && t.dataStrU8() == VA_ARG_STR) {
      Throw("{} encountered in text", VA_ARG_STR);
    }
    text.push_back(TextToken(t, {}));
  }
  replace(text);
  vector<PPToken> result;
  result.reserve(text.size());
  for (auto& t : text) {
    result.push_back(PPToken(t.token));
  }
  return result;
}

}
