#include "Declarator.h"
#include "DeclSpecifiers.h"

#define EX(func) #func, make_delegate(&SimpleDeclaration::func, this)

namespace compiler {

namespace SemanticParserImp {

using namespace std;

struct SimpleDeclaration : virtual Base {
  // TODO: initDeclaratorList can be omitted when declaring a class or
  // enumeration
  void simpleDeclaration() override {
    DeclSpecifiers declSpecifiers = TR(EX(declSpecifierSeq));
    TR(EX(initDeclaratorList), declSpecifiers);
    expect(OP_SEMICOLON);
  }

  DeclSpecifiers declSpecifierSeq() {
    DeclSpecifiers declSpecifiers;
    while (BT(EX(declSpecifier), declSpecifiers)) {
    }
    declSpecifiers.finalize();
    return declSpecifiers;
  }

  void declSpecifier(DeclSpecifiers& declSpecifiers) {
    if (tryAdvSimple(KW_TYPEDEF)) {
      declSpecifiers.setTypedef();
    } else if (!BT(EX(storageClassSpecifier), declSpecifiers)) {
      TR(EX(typeSpecifier), declSpecifiers);
    }
  }

  void storageClassSpecifier(DeclSpecifiers& declSpecifiers) {
    if (tryAdvSimple(KW_STATIC)) {
      declSpecifiers.addStorageClass(DeclSpecifiers::Static);
    } else if (tryAdvSimple(KW_THREAD_LOCAL)) {
      declSpecifiers.addStorageClass(DeclSpecifiers::ThreadLocal);
    } else {
      expect(KW_EXTERN);
      declSpecifiers.addStorageClass(DeclSpecifiers::Extern);
    }
  }

  void typeSpecifier(DeclSpecifiers& declSpecifiers) {
    TR(EX(trailingTypeSpecifier), declSpecifiers);
  }

  void trailingTypeSpecifier(DeclSpecifiers& declSpecifiers) {
    CvQualifier cv;
    if (BT(EX(cvQualifier), cv)) {
      declSpecifiers.addCvQualifier(cv);
    } else {
      TR(EX(simpleTypeSpecifier), declSpecifiers);
    }
  }

  void cvQualifier(CvQualifier& cv) {
    if (tryAdvSimple(KW_CONST)) {
      cv = CvQualifier::Const;
    } else {
      expect(KW_VOLATILE);
      cv = CvQualifier::Volatile;
    }
  }

  void simpleTypeSpecifier(DeclSpecifiers& declSpecifiers) {
    if (isSimple({
          KW_CHAR, KW_CHAR16_T, KW_CHAR32_T, KW_WCHAR_T, KW_BOOL, KW_SHORT,
          KW_INT, KW_LONG, KW_SIGNED, KW_UNSIGNED, KW_FLOAT, KW_DOUBLE,
          KW_VOID /*, KW_AUTO */
        })) {
      declSpecifiers.addType(
        make_shared<FundalmentalType>(getAdvSimple()->type), false);
    } else {
      // Note that here we could avoid the lookup by checking whether
      // declSpecifiers already contain a type
      // However the current implementation is correct and is most clean
      auto type = TR(EX(typeName));
      declSpecifiers.addType(type, true);
    }
  }

  SType typeName() {
    return TR(EX(typedefName));
  }

  SType typedefName() {
    auto name = expectIdentifier();
    auto member = curNamespace()->lookupTypedef(name, false);
    if (!member) {
      Throw("typedef name expected; got {}", name);
    }
    return member->type;
  }

  void initDeclaratorList(DeclSpecifiers& declSpecifiers) {
    TR(EX(initDeclarator), declSpecifiers);
    while (tryAdvSimple(OP_COMMA)) {
      TR(EX(initDeclarator), declSpecifiers);
    }
  }

  // TODO: initializer
  void initDeclarator(DeclSpecifiers& declSpecifiers) {
    auto declarator = TR(EX(declarator));
    declarator->appendType(declSpecifiers.getType());
    if (declSpecifiers.isTypedef()) {
      curNamespace()->addTypedef(declarator->getId(), 
                                 declarator->getType());
    } else {
      curNamespace()->addVariableOrFunction(declarator->getId(),
                                            declarator->getType());
    }
  }

  // TODO: noptr-declarator parameters-and-qualifiers trailing-return-type
  UDeclarator declarator() {
    return TR(EX(ptrDeclarator));
  }

  // TODO: a purely recursive parse may not be very efficient
  UDeclarator ptrDeclarator() {
    UPtrOperator op = BT(EX(ptrOperator));
    if (!op) {
      return TR(EX(noptrDeclarator));
    }

    UDeclarator declarator = TR(EX(ptrDeclarator));
    applyPtrOperator(declarator, op);
    return declarator;
  }

  // TODO: consider moving this to Declarator
  void applyPtrOperator(const UDeclarator& declarator, 
                        const UPtrOperator& op) const {
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

    declarator->appendType(type);
  }

  UPtrOperator ptrOperator() {
    if (tryAdvSimple(OP_AMP)) {
      return make_unique<PtrOperator>(PtrOperator::LValueRef);
    } else if (tryAdvSimple(OP_LAND)) {
      return make_unique<PtrOperator>(PtrOperator::RValueRef);
    } else {
      expect(OP_STAR);
      CvQualifier cvQualifiers, cv;
      while (BT(EX(cvQualifier), cv)) {
        cvQualifiers = cvQualifiers.combine(cv, true);
      }
      return make_unique<PtrOperator>(PtrOperator::Pointer, cvQualifiers);
    }
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
    declarator->appendType(arrayType);
  }

  void noptrDeclaratorSuffixFunction(const UDeclarator& declarator) {
    auto funcType = TR(EX(parametersAndQualifiers));
    declarator->appendType(funcType);
  }

  SType parametersAndQualifiers() {
    expect(OP_LPAREN);
    auto ret = TR(EX(parameterDeclarationClause));
    expect(OP_RPAREN);
    return ret;
  }

  SType parameterDeclarationClause() {
    SType ret;
    vector<SType> parameters;
    bool hasVarArgs = false;
    if (!isSimple(OP_RPAREN)) {
      if (tryAdvSimple(OP_DOTS)) {
        hasVarArgs = true;
      } else {
        parameters = TR(EX(parameterDeclarationList));
        if (tryAdvSimple(OP_COMMA)) {
          expect(OP_DOTS);
          hasVarArgs = true;
        } else if (tryAdvSimple(OP_DOTS)) {
          hasVarArgs = true;
        }
      }
    } // else: empty parameter list: '()'

    return make_shared<FunctionType>(move(parameters), hasVarArgs);
  }

  vector<SType> parameterDeclarationList() {
    vector<SType> params;
    params.push_back(TR(EX(parameterDeclaration)));
    while (isSimple(OP_COMMA) && !nextIsSimple(OP_DOTS)) {
      adv();
      params.push_back(TR(EX(parameterDeclaration)));
    }
    return params;
  }

  SType parameterDeclaration() {
    DeclSpecifiers declSpecifiers = TR(EX(declSpecifierSeq));
    // TODO: disallow typedef and storage class
    UDeclarator declarator;
    (declarator = BT(EX(declarator))) ||
    (declarator = BT(EX(abstractDeclarator))) ||
    (declarator = make_unique<Declarator>());
    declarator->appendType(declSpecifiers.getType());
    return declarator->getType();
  }

  UDeclarator abstractDeclarator() {
    return TR(EX(ptrAbstractDeclarator));
  }

  UDeclarator ptrAbstractDeclarator() {
    vector<UPtrOperator> ops;
    while (auto op = BT(EX(ptrOperator))) {
      ops.push_back(move(op));
    }
    auto declarator = BT(EX(noptrAbstractDeclarator));
    if (!declarator) {
      if (ops.empty()) {
        BAD_EXPECT("noptrAbstractDeclarator");
      }
      declarator = make_unique<Declarator>();
    }
    for (int i = ops.size() - 1; i >= 0; --i) {
      applyPtrOperator(declarator, ops[i]);
    }
    return declarator;
  }

  UDeclarator noptrAbstractDeclarator() {
    auto declarator = BT(EX(noptrAbstractDeclaratorRoot));
    if (!declarator) {
      declarator = make_unique<Declarator>();
      TR(EX(noptrDeclaratorSuffix), declarator);
    }

    while (BT(EX(noptrDeclaratorSuffix), declarator)) {
    }

    return declarator;
  }

  UDeclarator noptrAbstractDeclaratorRoot() {
    expect(OP_LPAREN);
    auto declarator = TR(EX(ptrAbstractDeclarator));
    expect(OP_RPAREN);
    return declarator;
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
