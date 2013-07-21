#pragma once
#include "PostProcessingToken.h"
#include <vector>

namespace compiler
{

// Print AST can use LISP style
struct ASTNode;
typedef std::unique_ptr<ASTNode> AST;
struct ASTNode
{
  ASTNode(const PostToken* t)
    : isTerminal(true),
      token(t) { }
  ASTNode(std::vector<AST>&& child)
    : isTerminal(false),
      token(nullptr),
      children(std::move(child)) { }
  bool isTerminal;
  const PostToken* token;
  std::vector<AST> children; 
};

class Parser
{
public:
  Parser(const std::vector<UToken>& tokens)
    : tokens_(tokens) { }
  AST process();
private:
  const std::vector<UToken>& tokens_;
};

}
