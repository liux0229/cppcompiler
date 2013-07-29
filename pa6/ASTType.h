#pragma once
#include <map>

namespace compiler {

namespace {

enum class ASTType
{
  Terminal, // we don't give a name to the wrapped terminal

  Identifier,

  ConstantExpression,
  ConditionalExpression,
  LogicalOrExpression,
  LogicalAndExpression,
  Expression,
  AssignmentExpression,
  AssignmentOperator,
  InclusiveOrExpression,
  ExclusiveOrExpression,
  AndExpression,
  EqualityExpression,
  EqualityOperator,
  RelationalExpression,
  RelationalOperator,
  ShiftExpression,
  ShiftOperator,
  AdditiveExpression,
  AdditiveOperator,
  MulplicativeExpression,
  MulplicativeOperator,
  PmExpression,
  PmOperator,
  CastExpression,
  CastOperator,
  UnaryExpression,
  TypeIdInParen, // helper non-terminal
  UnaryOperator,
  PostfixExpression,
  PostfixRoot,
  NoExceptExpression,
  PrimaryExpression,

  TypeId,

  NewExpression,
  DeleteEXpression,

  InitializerClause,
  BracedInitList,
  InitializerList,
  InitializerClauseDots,

  ThrowExpression,
};

#define Z(lhs, rhs) { ASTType::lhs, #rhs }
const std::map<ASTType, std::string> astTypeToString = {
  Z(Identifier, identifier),
  Z(ConstantExpression, constant-expression),
  Z(ConditionalExpression, conditional-expression),
  Z(LogicalOrExpression, logical-or-expression),
  Z(LogicalAndExpression, logical-and-expression),
  Z(Expression, expression),
  Z(AssignmentExpression, assignment-expression),
  Z(AssignmentOperator, assignment-operator),
  Z(InclusiveOrExpression, inclusive-or-expression),
  Z(ExclusiveOrExpression, exclusive-or-expression),
  Z(AndExpression, and-expression),
  Z(EqualityExpression, equality-expression),
  Z(EqualityOperator, equality-operator),
  Z(RelationalExpression, relational-expression),
  Z(RelationalOperator, relational-operator),
  Z(ShiftExpression, shift-expression),
  Z(ShiftOperator, shift-operator),
  Z(AdditiveExpression, additive-expression),
  Z(AdditiveOperator, additive-operator),
  Z(MulplicativeExpression, mulplicative-expression),
  Z(MulplicativeOperator, mulplicative-operator),
  Z(PmExpression, pm-expression),
  Z(PmOperator, pm-operator),
  Z(CastExpression, cast-expression),
  Z(CastOperator, cast-operator),
  Z(UnaryExpression, unary-expression),
  Z(TypeIdInParen, type-id-in-paren),
  Z(UnaryOperator, unary-operator),
  Z(PostfixExpression, postfix-expression),
  Z(PostfixRoot, postfix-root),
  Z(NoExceptExpression, no-except-expression),
  Z(PrimaryExpression, primary-expression),

  Z(TypeId, type-id),

  Z(NewExpression, new-expression),
  Z(DeleteEXpression, delete-eXpression),

  Z(InitializerClause, initializer-clause),
  Z(BracedInitList, braced-init-list),
  Z(InitializerList, initializer-list),
  Z(InitializerClauseDots, initializer-clause-dots),

  Z(ThrowExpression, throw-expression),
};
#undef Z

std::string getASTTypeName(ASTType type) {
  auto it = astTypeToString.find(type);
  MCHECK(it != astTypeToString.end(),
         format("{} is not defined in the astTypeToString map",
                static_cast<int>(type)));
  return it->second;
}

}

}

