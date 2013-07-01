#include "MacroProcessor.h"
#include "PPDirectiveUtil.h"
#include "PPTokenizer.h"
#include <string>
#include <memory>
#include <sstream>
#include <algorithm>
#include <functional>

namespace compiler {

using namespace std;

namespace {

size_t skipWhite(const vector<PPToken>& tokens, size_t i) {
  while (i < tokens.size() && tokens[i].isWhite()) {
    ++i;
  }
  return i;
}

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
      merged = token;
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

PPToken getLiteral(vector<PPToken>&& tokens) {
  vector<int> data;
  bool prevSpace = false;
  data.push_back('"');

  for (auto& t : tokens) {
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
        if (x == '\\' || x == '"') {
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
                    const vector<string>& parentMacros) const {
  TextList result;
  for (auto& repl : bodyList) {
    vector<TextToken> text;
    for (auto& r : repl) {
      text.push_back(TextToken(r.token, parentMacros));
    }
    result.push_back(move(text));
  }
  return result;
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
    } else if (name == "__VAR_ARGS__") {
      if (!varArg) {
        Throw("use __VAR_ARGS__ in replacement list of a macro which does not"
              " have the '...' parameter");
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
    if (text.empty() || result.empty()) {
      continue;
    }
    PPToken merged = mergeToken(result.back().token, text[0].token);
    result.back() = TextToken(merged, 
                              add(result.back().parentMacros,
                                  text[0].parentMacros));

    result.insert(result.end(), text.begin() + 1, text.end());
  }
  return result;
}

MacroProcessor::TextList
MacroProcessor::applyFunction(const string& name,
                              const Macro& macro,
                              vector<vector<PPToken>>&& args,
                              const vector<string>& parentMacros) {
  // TODO: figure out the rule for the number of parameters
  size_t nparam = macro.paramList.size();
  if (!macro.varArg) {
    if (nparam != args.size()) {
      Throw("wrong number of args for macro invocation for {}; "
            "{} expected, {} received",
            name,
            macro.paramList.size(),
            args.size());
    }
  } else {
    if (nparam > args.size()) {
      Throw("wrong number of args for macro invocation for {}; "
            ">= {} expected, {} received",
            name,
            macro.paramList.size(),
            args.size());
    }
    if (args.size() > nparam) {
      size_t last = nparam;
      for (size_t i = last + 1; i < args.size(); ++i) {
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
          param = nparam;
        }
        auto& arg = args[param];
        vector<TextToken> argText;

        if (r.literal) {
          argText.push_back(TextToken(getLiteral(move(arg)), {}));
        } else {
          for (auto& t : arg) {
            // cout << format("extract param {}: {}\n", param, t.dataStrU8());
            // TODO: we can move 't' here
            argText.push_back(TextToken(t, {}));
          }
        }

        if (r.inplaceExpand) {
          replace(argText);
        }

        // now substitue the argument
        for (auto& t : argText) {
          text.push_back(TextToken(t.token, add(t.parentMacros, parentMacros)));
        }
      } else {
        text.push_back(TextToken(r.token, parentMacros));
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
vector<MacroProcessor::TextToken>&
MacroProcessor::replace(vector<TextToken>& text)
{
  // debug(text);

  bool expanded;
  do {
    expanded = false;
    for (auto it = text.begin(); it != text.end(); ++it) {
      auto& t = it->token;
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
        auto result = merge(macro.getReplTextList(add(it->parentMacros, name)));
        it = text.erase(it);
        text.insert(it, result.begin(), result.end());
        expanded = true;
        break;
      } else {
        auto itEnd = it + 1;
        if (itEnd != text.end() && isLParen(itEnd->token)) {
          // find matching ')' and collect arguments
          int count = 1;
          vector<vector<PPToken>> args(1);
          for (++itEnd; itEnd != text.end(); ++itEnd) {
            if (isComma(itEnd->token) && count == 1) {
              args.push_back({});
            } else {
              if (!isRParen(itEnd->token) || count > 1) {
                args.back().push_back(itEnd->token);
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
          for (auto& arg : args) {
            trim<PPToken>(arg, [](const PPToken& token) {
              return token.isWhite() || token.type == PPTokenType::NewLine;
            });
          }

          auto result = merge(applyFunction(name, 
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

  return text;
}

vector<PPToken> MacroProcessor::expand(const vector<PPToken>& rawText)
{
  vector<TextToken> text;
  for (auto& t : rawText) {
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
