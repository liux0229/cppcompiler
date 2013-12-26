#include "Declarator.h"
#include "DeclSpecifiers.h"

#define EX(func) #func, make_delegate(&SimpleDeclaration::func, this)

namespace compiler {

namespace SemanticParserImp {

using namespace std;

struct SimpleDeclaration : virtual Base {
#if 0
  /* =============
   *  declarations
   * =============
   */
  AST declaration() {
    VAST c;
    AST node;
    (node = BT(emptyDeclaration)) ||
    (node = BT(blockDeclaration)) ||
    (node = TR(namespaceDefinition));
    c.push_back(move(node));
    return get(ASTType::Declaration, move(c));
  }

  AST blockDeclaration() {
    VAST c;
    AST node;
    (node = BT(simpleDeclaration)) ||
    (node = BT(namespaceAliasDefinition)) ||
    (node = BT(usingDeclaration)) ||
    (node = BT(usingDirective)) ||
    (node = TR(aliasDeclaration));
    c.push_back(move(node));
    return get(ASTType::BlockDeclaration, move(c));
  }
#endif

  // TODO: initDeclaratorList can be omitted when declaring a class or
  // enumeration
  void simpleDeclaration() override {
    DeclSpecifiers declSpecifiers = TR(EX(declSpecifierSeq));
    auto declarators = TR(EX(initDeclaratorList));
    expect(OP_SEMICOLON);

    for (auto& declarator : declarators) {
      declarator->typeList.append(declSpecifiers.type);
      cout << declarator->id << " " << *declarator->typeList.root << endl;
    }
  }

  DeclSpecifiers declSpecifierSeq() {
    DeclSpecifiers declSpecifiers;
    while (BT(EX(declSpecifier), declSpecifiers)) {
    }
    declSpecifiers.validate();
    return declSpecifiers;
  }

  void declSpecifier(DeclSpecifiers& declSpecifiers) {
    SType type = TR(EX(typeSpecifier));
    declSpecifiers.setType(type);
  }

  SType typeSpecifier() {
    return TR(EX(trailingTypeSpecifier));
  }

  SType trailingTypeSpecifier() {
    if (auto cv = BT(EX(cvQualifier))) {
      auto ret = make_shared<CvQualifierType>();
      ret->setCvQualifier(*cv);
      return ret;
    } else {
      return TR(EX(simpleTypeSpecifier));
    }
  }

  UCvQualifier cvQualifier() {
    if (tryAdvSimple(KW_CONST)) {
      return make_unique<CvQualifier>(CvQualifier::Const);
    } else {
      expect(KW_VOLATILE);
      return make_unique<CvQualifier>(CvQualifier::Volatile);
    }
  }

  SType simpleTypeSpecifier() {
    if (isSimple({
          KW_CHAR, KW_CHAR16_T, KW_CHAR32_T, KW_WCHAR_T, KW_BOOL, KW_SHORT,
          KW_INT, KW_LONG, KW_SIGNED, KW_UNSIGNED, KW_FLOAT, KW_DOUBLE,
          KW_VOID /*, KW_AUTO */
        })) {
      return make_shared<FundalmentalType>(getAdvSimple()->type);
    } else {
      Throw("Not implemented");
      // zeroOrOne(nestedNameSpecifier);
      // c.push_back(TR(typeName));
      return nullptr;
    }
  }

  vector<UDeclarator> initDeclaratorList() {
    vector<UDeclarator> ret;
    ret.push_back(TR(EX(initDeclarator)));
    while (tryAdvSimple(OP_COMMA)) {
      ret.push_back(TR(EX(initDeclarator)));
    }
    return ret;
  }

  // TODO: initializer
  UDeclarator initDeclarator() {
    return TR(EX(declarator));
  }

  // TODO: noptr-declarator parameters-and-qualifiers trailing-return-type
  UDeclarator declarator() {
    return TR(EX(ptrDeclarator));
  }

  UPtrOperator ptrOperator() {
    if (tryAdvSimple(OP_AMP)) {
      return make_unique<PtrOperator>(PtrOperator::LValueRef);
    } else if (tryAdvSimple(OP_LAND)) {
      return make_unique<PtrOperator>(PtrOperator::RValueRef);
    } else {
      expect(OP_STAR);
      CvQualifier cvQualifiers;
      while (auto cv = BT(EX(cvQualifier))) {
        cvQualifiers |= *cv;
      }
      return make_unique<PtrOperator>(PtrOperator::Pointer, cvQualifiers);
    }
  }

  // TODO: a purely recursive parse may not be very efficient
  UDeclarator ptrDeclarator() {
    UPtrOperator op = BT(EX(ptrOperator));
    if (!op) {
      return TR(EX(noptrDeclarator));
    }

    UDeclarator declarator = TR(EX(ptrDeclarator));
    SType type;
    switch (op->type) {
      case PtrOperator::LValueRef:
        type = make_shared<ReferenceType>(ReferenceType::LValueRef);
        break;
      case PtrOperator::RValueRef:
        type = make_shared<ReferenceType>(ReferenceType::RValueRef);
        break;
      case PtrOperator::Pointer:
        type = make_shared<PointerType>();
        type->setCvQualifier(op->cvQualifier);
        break;
    }

    declarator->typeList.append(type);
    return declarator;
  }

  UDeclarator noptrDeclarator() {
    UDeclarator declarator = TR(EX(noptrDeclaratorRoot));
    while (BT(EX(noptrDeclaratorSuffix), declarator)) {
    }
    return declarator;
  }

  UDeclarator noptrDeclaratorRoot() {
    if (tryAdvSimple(OP_LPAREN)) {
      auto declarator = TR(EX(ptrDeclarator));
      expect(OP_RPAREN);
      return declarator;
    } else {
      auto id = TR(EX(declaratorId));
      return make_unique<Declarator>(*id);
    }
  }

  UDeclaratorId declaratorId() {
    auto id = TR(EX(idExpression));
    return make_unique<DeclaratorId>(*id);
  }

  void noptrDeclaratorSuffix(const UDeclarator& declarator) {
    BT(EX(noptrDeclaratorSuffixArray), declarator) ||
    TR(EX(noptrDeclaratorSuffixFunction), declarator);
  }

  void noptrDeclaratorSuffixArray(const UDeclarator& declarator) {
    expect(OP_LSQUARE);
    size_t size = 0;
    if (!isSimple(OP_RSQUARE)) {
      size = TR(EXB(constantExpression));
    }
    expect(OP_RSQUARE);
    auto arrayType = make_shared<ArrayType>(size);
    declarator->typeList.append(arrayType);
  }

  void noptrDeclaratorSuffixFunction(const UDeclarator& declarator) {
    throw CompilerException("not implemented");
  }

#if 0
  AST namespaceAliasDefinition() {
    VAST c;
    c.push_back(expect(KW_NAMESPACE));
    c.push_back(expectIdentifier());
    c.push_back(expect(OP_ASS));
    c.push_back(TR(qualifiedNamespaceSpecifier));
    c.push_back(expect(OP_SEMICOLON));
    return getAST(NamespaceAliasDefinition);
  }

  AST qualifiedNamespaceSpecifier() {
    VAST c;
    zeroOrOne(nestedNameSpecifier);
    c.push_back(TR(namespaceName));
    return getAST(QualifiedNamespaceSpecifier);
  }

  AST usingDirective() {
    VAST c;
    c.push_back(expect(KW_USING));
    c.push_back(expect(KW_NAMESPACE));
    zeroOrOne(nestedNameSpecifier);
    c.push_back(expectIdentifier());
    c.push_back(expect(OP_SEMICOLON));
    return getAST(UsingDirective);
  }

  AST namespaceDefinition() {
    VAST c;
    if (isSimple(KW_INLINE)) {
      c.push_back(getAdv());
    }
    c.push_back(expect(KW_NAMESPACE));
    if (isIdentifier()) {
      c.push_back(getAdv(ASTType::Identifier));
    }
    c.push_back(expect(OP_LBRACE));
    c.push_back(TR(namespaceBody));
    c.push_back(expect(OP_RBRACE));
    return getAST(NamespaceDefinition);
  }

  AST namespaceBody() {
    VAST c;
    zeroOrMore(declaration);
    return getAST(NamespaceBody);
  }

  AST emptyDeclaration() {
    // give it a separate layer
    return expectM(ASTType::EmptyDeclaration, 
                   "empty declaration", 
                   {OP_SEMICOLON});
  }

  AST parametersAndQualifiers() {
    VAST c;
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(parameterDeclarationClause));
    c.push_back(expect(OP_RPAREN));
    // TODO:
    // zeroOrMore(cvQualifier);
    // zeroOrOne(refQualifier);
    // zeroOrOne(exceptionSpecification);
    // zeroOrMore(attributeSpecifier);
    return get(ASTType::ParametersAndQualifiers, move(c));
  }

  AST parameterDeclarationClause() {
    AST node;
    // prefer to reduce the non-(potentially)-empty version first
    (node = BT(parameterDeclarationClauseA)) ||
    (node = BT(parameterDeclarationClauseB));
    return node;
  }

  AST parameterDeclarationClauseA() {
    VAST c;
    c.push_back(TR(parameterDeclarationList));
    c.push_back(expect(OP_COMMA));
    c.push_back(expect(OP_DOTS));
    return getAST(ParameterDeclarationClause);
  }

  AST parameterDeclarationClauseB() {
    VAST c;
    zeroOrOne(parameterDeclarationList);
    if (isSimple(OP_DOTS)) {
      c.push_back(getAdv());
    }
    return getAST(ParameterDeclarationClause);
  }

  AST parameterDeclarationList() {
    // parameter-declaration-list's FOLLOW contains OP_COMMA
    // so need special handling
    // TODO: we need a generic way of ensuring that these cases are handled
    // correctly
    // We can probably directly do a backtracking here instead
    VAST c;
    c.push_back(TR(parameterDeclaration));
    while (isSimple(OP_COMMA) && !nextIsSimple(OP_DOTS)) {
      c.push_back(getAdv());
      c.push_back(TR(parameterDeclaration));
    }
    return getAST(ParameterDeclarationList);
  }

  AST parameterDeclaration() {
    VAST c;
    c.push_back(TR(declSpecifierSeq));
    AST node;
    // TODO: do we need to disambuiguate here?
    // TODO: how to make this efficient (e.g. how do we know there is no
    // declarator - or we can discover that while we are parsing)?
    (node = BT(parameterDeclarationSuffixA)) ||
    (node = BT(parameterDeclarationSuffixB));
    c.push_back(move(node));
    return getAST(ParameterDeclaration);
  }

  AST parameterDeclarationSuffixA() {
    VAST c;
    c.push_back(TR(declarator));
    return getAST(ParameterDeclarationSuffix);
  }

  AST parameterDeclarationSuffixB() {
    VAST c;
    // TODO: if there is one we always take it; examine whether there are
    // cases we miss because of this
    // NOTE: in the entire implementation we'd assume this would not happen
    // (some can be obvious, others we didn't check)
    // Need more rigor around this
    zeroOrOne(abstractDeclarator);
    return getAST(ParameterDeclarationSuffix);
  }

  AST typeId() {
    VAST c;
    c.push_back(TR(typeSpecifierSeq));
    zeroOrOne(abstractDeclarator);
    return get(ASTType::TypeId, move(c));
  }

  AST abstractDeclarator() {
    VAST c;
    c.push_back(TR(ptrAbstractDeclarator));
    // TODO: trailing-return-type && abstract-pack-declarator
    return get(ASTType::AbstractDeclarator, move(c));
  }

  AST ptrAbstractDeclarator() {
    // TODO: we do not seem to be able to determine which path to take by using
    // FIRST / FOLLOW alone. So do generic BT
    // TODO: I think we might be able to simplify this
    AST node;
    (node = BT(ptrAbstractDeclaratorA)) ||
    (node = TR(ptrAbstractDeclaratorB));
    return node;
  }

  AST ptrAbstractDeclaratorA() {
    VAST c;
    zeroOrMore(ptrOperator);
    c.push_back(TR(noptrAbstractDeclarator));
    return get(ASTType::PtrAbstractDeclarator, move(c));
  }

  AST ptrAbstractDeclaratorB() {
    VAST c;
    oneOrMore(ptrOperator);
    return get(ASTType::PtrAbstractDeclarator, move(c));
  }

  AST noptrAbstractDeclarator() {
    VAST c;
    if (AST node = BT(noptrAbstractDeclaratorRoot)) {
      c.push_back(move(node));
      zeroOrMore(noptrDeclaratorSuffix);
    } else {
      oneOrMore(noptrDeclaratorSuffix);
    }
    return get(ASTType::NoptrAbstractDeclarator, move(c));
  }

  AST noptrAbstractDeclaratorRoot() {
    VAST c;
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(ptrAbstractDeclarator));
    c.push_back(expect(OP_RPAREN));
    return getAST(NoptrAbstractDeclaratorRoot);
  }
#endif

  /* ===============
   *  id expression
   * ===============
   */
  UId idExpression() {
    auto id = expectIdentifier();
    return make_unique<Id>(id);
    // TODO: make the parsing more effective
#if 0
    AST node;
    (node = BT(qualifiedId)) ||
    (node = TR(unqualifiedId));
    VAST c;
    c.push_back(move(node));
    return get(ASTType::IdExpression, move(c));
#endif
  }

#if 0
  AST unqualifiedId() {
    VAST c;
    c.push_back(expectIdentifier());
    return getAST(UnqualifiedId);
  }

  AST typeSpecifierSeq() {
    VAST c;
    oneOrMore(typeSpecifier);
    return get(ASTType::TypeSpecifierSeq, move(c));
  }


  AST nestedNameSpecifier() {
    VAST c;
    c.push_back(TR(nestedNameSpecifierRoot));
    zeroOrMore(nestedNameSpecifierSuffix);
    return get(ASTType::NestedNameSpecifier, move(c));
  }

  AST nestedNameSpecifierRoot() {
    // TODO: decltype
    VAST c;
    zeroOrOne(namespaceName);
    c.push_back(expect(OP_COLON2));
    return get(ASTType::NestedNameSpecifierRoot, move(c));
  }

  AST nestedNameSpecifierSuffix() {
    VAST c;
    c.push_back(expectIdentifier());
    c.push_back(expect(OP_COLON2));
    return get(ASTType::NestedNameSpecifierSuffix, move(c));
  }

  AST storageClassSpecifier() {
    return 
      expectM(ASTType::StorageClassSpecifier,
              "storage class specifier",
              {KW_STATIC, KW_THREAD_LOCAL, KW_EXTERN});
  }

  AST usingDeclaration() {
    VAST c;
    c.push_back(expect(KW_USING));
    c.push_back(TR(nestedNameSpecifier));
    c.push_back(TR(unqualifiedId));
    c.push_back(expect(OP_SEMICOLON));
    return get(ASTType::UsingDeclaration, move(c));
  }

  AST aliasDeclaration() {
    VAST c;
    c.push_back(expect(KW_USING));
    c.push_back(expectIdentifier());
    c.push_back(expect(OP_ASS));
    c.push_back(TR(typeId));
    c.push_back(expect(OP_SEMICOLON));
    return get(ASTType::AliasDeclaration, move(c));
  } 

  AST typeName() {
    // An important NT - give it a level
    VAST c;
    c.push_back(TR(typedefName));
    return get(ASTType::TypeName, move(c));
  }

  AST namespaceName() {
    if (!isNamespaceName()) {
      BAD_EXPECT("namespace name");
    }
    // should be fine to not use the Identifier type
    return getAdv(ASTType::NamespaceName);
  }

  AST typedefName() {
    if (!isTypedefName()) {
      BAD_EXPECT("typedef name");
    }
    return getAdv(ASTType::TypedefName);
  }

  AST qualifiedId() {
    VAST c;
    c.push_back(TR(nestedNameSpecifier));
    c.push_back(TR(unqualifiedId));
    return getAST(QualifiedId);
  }
#endif
};

} }

#undef EX
