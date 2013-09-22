#pragma once
#include "PostProcessingToken.h"
#include "ASTType.h"
#include <vector>

namespace compiler
{

// Print AST can use LISP style
struct ASTNode;
typedef std::unique_ptr<ASTNode> AST;
struct ASTNode
{
  ASTNode(ASTType _type, const PostToken* _token)
    : type(_type),
      isTerminal(true),
      token(_token) { }
  ASTNode(ASTType _type, std::vector<AST>&& _children)
    : type(_type),
      isTerminal(false),
      token(nullptr),
      children(std::move(_children)) { 
    for (auto& c : children) {
      MCHECK(
         c,
         format("null child encountered while creating {}",
                getASTTypeName(type)));
    }
  }

  std::string toStr(std::string indent, bool collapse) const;
  std::string toStr(bool collapse = false) const {
    return toStr("", collapse);
  }

  ASTType type;
  bool isTerminal;
  const PostToken* token;
  std::vector<AST> children; 
};

class Parser
{
public:
  struct Option
  {
    bool isTrace { false };
    bool isCollapse { true };
  };
  Parser(const std::vector<UToken>& tokens, const Option& option)
    : tokens_(tokens),
      option_(option) { }
  AST process();
private:
  const std::vector<UToken>& tokens_;
  const Option option_;
};

}
