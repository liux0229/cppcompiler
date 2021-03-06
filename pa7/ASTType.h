#pragma once
#include <map>

namespace compiler {

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
  ClassHeadName,
  MemInitialierId,
  CompoundStatement,
  FunctionTryBlock,
  Handler,

  Statement,
  LabeledStatement,
  ExpressionStatement,
  SelectionStatement,
  IterationStatement,
  ForTraditionalSpecifier,
  ForRangeBasedSpecifier,
  ForInitStatement,
  ForRangeDeclaration,
  ForRangeInitializer,
  JumpStatement,
  TryBlock,
  Condition,

  DeclSpecifierSeq,
  DeclSpecifier,
  StorageClassSpecifier,
  FunctionSpecifier,
  TrailingTypeSpecifier,
  SimpleTypeSpecifier,
  ElaboratedTypeSpecifier,
  ElaboratedTypeSpecifierA,
  ElaboratedTypeSpecifierB,
  TypenameSpecifier,
  CvQualifier,
  AttributeSpecifier,
  AttributeList,
  AttributePart,
  Attribute,
  AttributeToken,
  AttributeScopedToken,
  AttributeNamespace,
  PtrOperator,
  Declaration,
  BlockDeclaration,
  SimpleDeclaration,
  InitDeclarator,
  Declarator,
  PtrDeclarator,
  NoptrDeclarator,
  NoptrDeclaratorRoot,
  NoptrDeclaratorSuffix,
  ParametersAndQualifiers,
  ExceptionSpecification,
  TypeIdList,
  DynamicExceptionSpecification,
  AbstractDeclarator,
  PtrAbstractDeclarator,
  AbstractPackDeclarator,
  NoptrAbstractDeclarator,
  NoptrAbstractDeclaratorRoot,
  TrailingTypeSpecifierSeq,
  NoexceptSpecification,
  NoexceptSpecificationSuffix,
  InitDeclaratorList,
  Initializer,
  DeclaratorId,
  RefQualifier,
  TypeIdDots,
  TrailingReturnType,
  AsmDefinition,
  NamespaceAliasDefinition,
  QualifiedNamespaceSpecifier,
  UsingDirective,
  ExplicitInstantiation,
  OpaqueEnumDeclaration,
  ExplicitSpecialization,
  LinkageSpecification,
  NamespaceDefinition,
  NamespaceBody,
  EmptyDeclaration,
  AttributeDeclaration,
  ParameterDeclarationClause,
  ParameterDeclarationList,
  ParameterDeclaration,
  ParameterDeclarationSuffix,
  ExceptionDeclaration,
  ConditionDeclaration,
  DeclarationStatement,
  AttributeArgumentClause,
  BalancedToken,
  TemplateParameterList,
  TemplateParameter,
  TypeParameterSuffix,
  TypeParameter,
  TemplateId,

  LambdaExpression,
  LambdaIntroducer,
  LambdaCapture,
  CaptureListAtom,
  CaptureList,
  Capture,
  LambdaDeclarator,
  NewPlacement,
  NewTypeId,
  NewDeclarator,
  NoptrNewDeclarator,
  NoptrNewDeclaratorSuffix,
  DeleteExpression,
  CaptureDefault,
  NewInitializer,
  AlignmentSpecifier,
  PseudoDestructorName,
  PostfixSuffix,
  TranslationUnit,
  /* ===
     === */
};

namespace {

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
  Z(ClassHeadName, class-head-name),
  Z(MemInitialierId, mem-initialier-id),
  Z(CompoundStatement, compound-statement),
  Z(FunctionTryBlock, function-try-block),
  Z(Handler, handler),

  Z(Statement, statement),
  Z(LabeledStatement, labeled-statement),
  Z(ExpressionStatement, expression-statement),
  Z(SelectionStatement, selection-statement),
  Z(IterationStatement, iteration-statement),
  Z(ForTraditionalSpecifier, for-traditional-specifier),
  Z(ForRangeBasedSpecifier, for-range-based-specifier),
  Z(ForInitStatement, for-init-statement),
  Z(ForRangeDeclaration, for-range-declaration),
  Z(ForRangeInitializer, for-range-initializer),
  Z(JumpStatement, jump-statement),
  Z(TryBlock, try-block),
  Z(Condition, condition),

  Z(DeclSpecifierSeq, decl-specifier-seq),
  Z(DeclSpecifier, decl-specifier),
  Z(StorageClassSpecifier, storage-class-specifier),
  Z(FunctionSpecifier, function-specifier),
  Z(TrailingTypeSpecifier, trailing-type-specifier),
  Z(SimpleTypeSpecifier, simple-type-specifier),
  Z(ElaboratedTypeSpecifier, elaborated-type-specifier),
  Z(ElaboratedTypeSpecifierA, elaborated-type-specifier-a),
  Z(ElaboratedTypeSpecifierB, elaborated-type-specifier-b),
  Z(TypenameSpecifier, typename-specifier),
  Z(CvQualifier, cv-qualifier),
  Z(AttributeSpecifier, attribute-specifier),
  Z(AttributeList, attribute-list),
  Z(AttributePart, attribute-part),
  Z(Attribute, attribute),
  Z(AttributeToken, attribute-token),
  Z(AttributeScopedToken, attribute-scoped-token),
  Z(AttributeNamespace, attribute-namespace),
  Z(PtrOperator, ptr-operator),
  Z(Declaration, declaration),
  Z(BlockDeclaration, block-declaration),
  Z(SimpleDeclaration, simple-declaration),
  Z(InitDeclarator, init-declarator),
  Z(Declarator, declarator),
  Z(PtrDeclarator, ptr-declarator),
  Z(NoptrDeclarator, noptr-declarator),
  Z(NoptrDeclaratorRoot, noptr-declarator-root),
  Z(NoptrDeclaratorSuffix, noptr-declarator-suffix),
  Z(ParametersAndQualifiers, parameters-and-qualifiers),
  Z(ExceptionSpecification, exception-specification),
  Z(TypeIdList, type-id-list),
  Z(DynamicExceptionSpecification, dynamic-exception-specification),
  Z(AbstractDeclarator, abstract-declarator),
  Z(PtrAbstractDeclarator, ptr-abstract-declarator),
  Z(AbstractPackDeclarator, abstract-pack-declarator),
  Z(NoptrAbstractDeclarator, noptr-abstract-declarator),
  Z(NoptrAbstractDeclaratorRoot, noptr-abstract-declarator-root),
  Z(TrailingTypeSpecifierSeq, trailing-type-specifier-seq),
  Z(NoexceptSpecification, noexcept-specification),
  Z(NoexceptSpecificationSuffix, noexcept-specification-suffix),
  Z(InitDeclaratorList, init-declarator-list),
  Z(Initializer, initializer),
  Z(DeclaratorId, declarator-id),
  Z(RefQualifier, ref-qualifier),
  Z(TypeIdDots, type-id-dots),
  Z(TrailingReturnType, trailing-return-type),
  Z(AsmDefinition, asm-definition),
  Z(NamespaceAliasDefinition, namespace-alias-definition),
  Z(QualifiedNamespaceSpecifier, qualified-namespace-specifier),
  Z(UsingDirective, using-directive),
  Z(ExplicitInstantiation, explicit-instantiation),
  Z(OpaqueEnumDeclaration, opaque-enum-declaration),
  Z(ExplicitSpecialization, explicit-specialization),
  Z(LinkageSpecification, linkage-specification),
  Z(NamespaceDefinition, namespace-definition),
  Z(NamespaceBody, namespace-body),
  Z(EmptyDeclaration, empty-declaration),
  Z(AttributeDeclaration, attribute-declaration),
  Z(ParameterDeclarationClause, parameter-declaration-clause),
  Z(ParameterDeclarationList, parameter-declaration-list),
  Z(ParameterDeclaration, parameter-declaration),
  Z(ParameterDeclarationSuffix, parameter-declaration-suffix),
  Z(ExceptionDeclaration, exception-declaration),
  Z(ConditionDeclaration, condition-declaration),
  Z(DeclarationStatement, declaration-statement),
  Z(AttributeArgumentClause, attribute-argument-clause),
  Z(BalancedToken, balanced-token),
  Z(TemplateParameterList, template-parameter-list),
  Z(TemplateParameter, template-parameter),
  Z(TypeParameterSuffix, type-parameter-suffix),
  Z(TypeParameter, type-parameter),
  Z(TemplateId, template-id),
  Z(LambdaExpression, lambda-expression),
  Z(LambdaIntroducer, lambda-introducer),
  Z(LambdaCapture, lambda-capture),
  Z(CaptureListAtom, capture-list-atom),
  Z(CaptureList, capture-list),
  Z(Capture, capture),
  Z(LambdaDeclarator, lambda-declarator),
  Z(NewPlacement, new-placement),
  Z(NewTypeId, new-type-id),
  Z(NewDeclarator, new-declarator),
  Z(NoptrNewDeclarator, noptr-new-declarator),
  Z(NoptrNewDeclaratorSuffix, noptr-new-declarator-suffix),
  Z(DeleteExpression, delete-expression),
  Z(CaptureDefault, capture-default),
  Z(NewInitializer, new-initializer),
  Z(AlignmentSpecifier, alignment-specifier),
  Z(PseudoDestructorName, pseudo-destructor-name),
  Z(PostfixSuffix, postfix-suffix),
  Z(TranslationUnit, translation-unit),
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

