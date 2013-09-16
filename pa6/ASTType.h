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

  IdExpression,
  UnqualifiedId,
  QualifiedId,
  DecltypeSpecifier,
  CloseAngleBracket,
  TemplateArgumentList,
  TemplateArgumentDots,
  TemplateArgument,
  OperatorFunctionId,
  LiteralOperatorId,
  ConversionFunctionId,
  ConversionTypeId,

  TypeSpecifierSeq,
  TypeSpecifier,
  EnumSpecifier,
  EnumHead,
  EnumBase,
  EnumKey,
  EnumeratorList,
  EnumeratorDefinition,
  NestedNameSpecifier,
  NestedNameSpecifierRoot,
  NestedNameSpecifierSuffix,

  TypeName,
  ClassName,
  EnumName,
  TypedefName,
  SimpleTemplateId,
  NamespaceName,

  StFinal,
  StOverride,
  StZero,

  ClassSpecifier,
  ClassHead,
  ClassKey,
  BaseClause,
  BaseSpecifierList,
  BaseSpecifierDots,
  AccessSpecifier,
  BaseSpecifer,
  BaseTypeSpecifier,
  ClassOrDecltype,
  MemberSpecification,
  MemberDeclaration,
  UsingDeclaration,
  StaticAssertDeclaration,
  AliasDeclaration,
  VirtSpecifier,
  FunctionDefinition,
  FunctionBody,
  CtorInitializer,
  MemInitialier,
  MemInitialierList,
  MemInitialierDots,
  TemplateDeclaration,
  MemberDeclaratorList,
  MemberDeclarator,
  PureSpecifier,
  BraceOrEqualInitializer,
  /* ===
     === */
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

  Z(IdExpression, id-expression),
  Z(UnqualifiedId, unqualified-id),
  Z(QualifiedId, qualified-id),
  Z(DecltypeSpecifier, decltype-specifier),
  Z(CloseAngleBracket, close-angle-bracket),
  Z(TemplateArgumentList, template-argument-list),
  Z(TemplateArgumentDots, template-argument-dots),
  Z(TemplateArgument, template-argument),
  Z(OperatorFunctionId, operator-function-id),
  Z(LiteralOperatorId, literal-operator-id),
  Z(ConversionFunctionId, conversion-function-id),
  Z(ConversionTypeId, conversion-type-id),

  Z(TypeSpecifierSeq, type-specifier-seq),
  Z(TypeSpecifier, type-specifier),
  Z(EnumSpecifier, enum-specifier),
  Z(EnumHead, enum-head),
  Z(EnumBase, enum-base),
  Z(EnumKey, enum-key),
  Z(EnumeratorList, enumerator-list),
  Z(EnumeratorDefinition, enumerator-definition),
  Z(NestedNameSpecifier, nestedName-specifier),
  Z(NestedNameSpecifierRoot, nested-name-specifier-root),
  Z(NestedNameSpecifierSuffix, nested-name-specifier-suffix),

  Z(TypeName, type-name),
  Z(ClassName, class-name),
  Z(EnumName, enum-name),
  Z(TypedefName, typedef-name),
  Z(SimpleTemplateId, simple-template-id),
  Z(NamespaceName, namespace-name),

  Z(StFinal, ST_FINAL),
  Z(StOverride, ST_OVERRIDE),
  Z(StZero, ST_ZERO),

  Z(ClassSpecifier, class-specifier),
  Z(ClassHead, class-head),
  Z(ClassKey, class-key),
  Z(BaseClause, base-clause),
  Z(BaseSpecifierList, base-specifier-list),
  Z(BaseSpecifierDots, base-specifier-dots),
  Z(AccessSpecifier, access-specifier),
  Z(BaseSpecifer, base-specifer),
  Z(BaseTypeSpecifier, base-type-specifier),
  Z(ClassOrDecltype, class-or-decltype),
  Z(MemberSpecification, member-specification),
  Z(MemberDeclaration, member-declaration),
  Z(UsingDeclaration, using-declaration),
  Z(StaticAssertDeclaration, static-assert-declaration),
  Z(AliasDeclaration, alias-declaration),
  Z(VirtSpecifier, virt-specifier),
  Z(FunctionDefinition, function-definition),
  Z(FunctionBody, function-body),
  Z(CtorInitializer, ctor-initializer),
  Z(MemInitialier, mem-initialier),
  Z(MemInitialierList, mem-initialier-list),
  Z(MemInitialierDots, mem-initialier-dots),
  Z(TemplateDeclaration, template-declaration),
  Z(MemberDeclaratorList, member-declarator-list),
  Z(MemberDeclarator, member-declarator),
  Z(PureSpecifier, pure-specifier),
  Z(BraceOrEqualInitializer, brace-or-equal-initializer),
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

