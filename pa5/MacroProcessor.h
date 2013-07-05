#pragma once
#include "PreprocessingToken.h"
#include <vector>
#include <map>
#include <string>

namespace compiler {
class MacroProcessor 
{
public:
  struct TextToken {
    explicit TextToken(const PPToken& _token, 
                       const std::vector<std::string>& _parentMacros)
      : token(_token),
        parentMacros(_parentMacros)  { }
    bool canExpand();
    PPToken token;
    std::vector<std::string> parentMacros;
    bool expand { true };
  };
private:
  typedef std::vector<std::vector<TextToken>> TextList;

  struct Repl {
    explicit Repl(const PPToken t, int param)
      : token(t),
        parameter(param) { }
    bool isParam() const {
      return parameter != -1;
    }

    std::string toStr() const;

    PPToken token;
    int parameter; // -2 indicates __VAR_ARGS__
    bool inplaceExpand { true };
    bool literal { false };
  };
  typedef std::vector<std::vector<Repl>> ReplList;

  struct Macro {
    enum class Type {
      Object,
      Function
    };

    Macro() { }
    Macro(Type _type,
          std::vector<std::string>&& _paramList,
          bool _varArg,
          ReplList&& _bodyList,
          std::vector<PPToken>&& _bodyOriginal,
          bool predefined = false)
      : type(_type),
        paramList(_paramList),
        varArg(_varArg),
        bodyList(_bodyList),
        bodyOriginal(_bodyOriginal),
        predefined_(predefined) { }

    bool operator==(const Macro& rhs) const {
      return type == rhs.type &&
             paramList == rhs.paramList &&
             varArg == rhs.varArg &&
             bodyOriginal == rhs.bodyOriginal;
    }

    bool operator!=(const Macro& rhs) const {
      return !(*this == rhs);
    }

    bool isObject() const { return type == Type::Object; }

    TextList getReplTextList(
              const std::string& name,
              const std::vector<std::string>& parentMacros) const;

    // debugging
    std::string toStr() const;

    Type type;
    std::vector<std::string> paramList; // utf8 data of identifier
    bool varArg { false }; 
    // elements in the list are separated by '##'
    ReplList bodyList; 
    // use this to check whether the replacement lists are identifical
    // TODO: get rid of this
    std::vector<PPToken> bodyOriginal;
    bool predefined_;
  };

public:
  MacroProcessor();
  void def(const std::vector<PPToken>& tokens);
  std::vector<PPToken> expand(const std::vector<PPToken>& text);
  bool isDefined(const std::string& identifier) const {
    return macros_.find(identifier) != macros_.end();
  }
private:
  void define(const std::vector<PPToken>& tokens);
  void undefine(const std::vector<PPToken>& tokens);
  size_t parseParam(const std::vector<PPToken>& tokens, 
                    size_t i, 
                    std::vector<std::string>* paramList,
                    bool* varArg);
  Repl mapParameter(const PPToken& token, 
                    const std::vector<std::string>& paramList,
                    bool varArg);
  std::vector<Repl> parseRepl(std::vector<Repl>&& repl);
  std::vector<TextToken>& replace(std::vector<TextToken>& text);
  std::vector<TextToken> merge(TextList&& textList);
  TextList applyFunction(const std::string& name,
                         const Macro& macro,
                         std::vector<std::vector<TextToken>>&& args,
                         const std::vector<std::string>& parentMacros);

  // name to macro
  // name is utf8 data of identifier
  std::map<std::string, Macro> macros_;  
};
}
