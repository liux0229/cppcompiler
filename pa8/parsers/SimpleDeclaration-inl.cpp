#include "Declarator.h"
#include "DeclSpecifiers.h"
#include "Initializer.h"

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
    } else if (tryAdvSimple(KW_CONSTEXPR)) {
      declSpecifiers.setConstExpr();
    } else {
      BT(EX(functionSpecifier), declSpecifiers) ||
      BT(EX(storageClassSpecifier), declSpecifiers) ||
      TR(EX(typeSpecifier), declSpecifiers);
    }
  }

  void functionSpecifier(DeclSpecifiers& declSpecifiers) {
    expect(KW_INLINE);
    declSpecifiers.setInline();
  }

  void storageClassSpecifier(DeclSpecifiers& declSpecifiers) {
    if (tryAdvSimple(KW_STATIC)) {
      declSpecifiers.addStorageClass(StorageClass::Static);
    } else if (tryAdvSimple(KW_THREAD_LOCAL)) {
      declSpecifiers.addStorageClass(StorageClass::ThreadLocal);
    } else {
      expect(KW_EXTERN);
      declSpecifiers.addStorageClass(StorageClass::Extern);
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
      auto ns = BT(EXB(nestedNameSpecifier));
      // Note that here we could avoid the lookup by checking whether
      // declSpecifiers already contain a type
      // However the current implementation is correct and is most clean
      auto type = TR(EX(typeName), ns);
      declSpecifiers.addType(type, true);
    }
  }

  SType typeName(Namespace* ns) {
    return TR(EX(typedefName), ns);
  }

  SType typedefName(Namespace* ns) {
    auto name = expectIdentifier();
    STypedefMember member;
    if (ns) {
      member = ns->lookupTypedef(name, true);
    } else {
      member = curNamespace()->lookupTypedef(name, false);
    }
    if (!member) {
      Throw("typedef name expected; got {}{}", 
            ns ? ns->getName() + "::" : string{},
            name);
    }
    return member->type;
  }

  void initDeclaratorList(const DeclSpecifiers& declSpecifiers) {
    TR(EX(initDeclarator), declSpecifiers);
    while (tryAdvSimple(OP_COMMA)) {
      TR(EX(initDeclarator), declSpecifiers);
    }
  }

  // get the namespace associated with a specific declarator id
  Namespace* getTargetNamespace(Id id) {
    if (!id.isQualified()) {
      return curNamespace();
    } else {
      if (id.ns->enclosedBy(curNamespace())) {
        return id.ns;
      } else {
        Throw("Declaring {} in {} not allowed: {} does not enclose {}",
              id.getName(),
              curNamespace()->getName(),
              curNamespace()->getName(),
              id.ns->getName());
        return nullptr;
      }
    }
  }

  void initDeclarator(const DeclSpecifiers& declSpecifiers) {
    auto frame = translationUnit_->saveFrame();
    auto declarator = TR(EX(declarator), declSpecifiers);
    auto initializer = BT(EX(initializer));
    translationUnit_->restoreFrame(frame);

    auto id = declarator->getId();
    if (declSpecifiers.isTypedef()) {
      if (id.isQualified()) {
        Throw("typedef declarator id cannot be a qualified id: {}",
              id.getName());
      }
      if (initializer) {
        Throw("typedef cannot have initializer: {}", id.getName());
      }
      curNamespace()->addTypedef(id.unqualified,
                                 declarator->getType());
    } else {
      auto target = getTargetNamespace(id);
      auto type = declarator->getType();
      bool requirePriorDeclaration = id.isQualified();
      if (type->isFunction()) {
        if (declSpecifiers.isConstExpr()) {
          Throw("constexpr cannot be used for function declarations: {}",
                id.getName());
        }
        if (initializer) {
          Throw("function declaration cannot have initializer", id.getName());
        }
        target->addFunction(id.unqualified,
                            static_pointer_cast<FunctionType>(type),
                            requirePriorDeclaration,
                            false, /* isDef */
                            declSpecifiers);
      } else {
        if (declSpecifiers.isInline()) {
          Throw("inline cannot be used for variable declarations: {}",
                id.getName());
        }
        if (type->isVoid()) {
          Throw("{} cannot have type {}", id.unqualified, *type);
        }

        target->addVariable(id.unqualified,
                            type,
                            requirePriorDeclaration,
                            declSpecifiers,
                            move(initializer));
      }
    }
  }

  UInitializer initializer() {
    expect(OP_ASS);
    auto expr = TR(EXB(expression));
    return make_unique<Initializer>(Initializer::Copy, expr);
  }

  // TODO: noptr-declarator parameters-and-qualifiers trailing-return-type
  UDeclarator declarator(const DeclSpecifiers& declSpecifiers) {
    auto declarator = TR(EX(ptrDeclarator));
    declarator->appendType(declSpecifiers.getType());
    return declarator;
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
      auto id = TR(EXB(idExpression));
      if (id->isQualified()) {
        translationUnit_->openNamespace(id->ns);
      }
      return make_unique<Declarator>(*id);
    }
  }

  void noptrDeclaratorSuffix(const UDeclarator& declarator) {
    BT(EX(noptrDeclaratorSuffixArray), declarator) ||
    TR(EX(noptrDeclaratorSuffixFunction), declarator);
  }

  void noptrDeclaratorSuffixArray(const UDeclarator& declarator) {
    expect(OP_LSQUARE);
    size_t size = 0;
    if (!isSimple(OP_RSQUARE)) {
      auto literal = TR(EXB(constantExpression));
      auto value = literal->value;
      if (!value->type->isFundalmental() ||
          !value->type->toFundalmental()->isIntegral()) {
        Throw("expect integral expression: {}", *literal);
      }
      auto fundalmental = dynamic_cast<FundalmentalValueBase*>(value.get());
      if (!fundalmental->isPositive()) {
        Throw("expect positive value: {}", *literal);
      }
      size = fundalmental->getValue();
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
    if ((declarator = BT(EX(declarator), declSpecifiers)) ||
        (declarator = BT(EX(abstractDeclarator), declSpecifiers))) {
      return declarator->getType();
    } else {
      return declSpecifiers.getType();
    }
  }

  UDeclarator abstractDeclarator(const DeclSpecifiers& declSpecifiers) {
    auto declarator = TR(EX(ptrAbstractDeclarator));
    declarator->appendType(declSpecifiers.getType());
    return declarator;
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

  SType typeId() {
    auto typeSpecifiers = TR(EX(typeSpecifierSeq));
    UDeclarator declarator;
    if (UDeclarator declarator = BT(EX(abstractDeclarator), typeSpecifiers)) {
      declarator->appendType(typeSpecifiers.getType());
      return declarator->getType();
    } else {
      return typeSpecifiers.getType();
    }
  }

  DeclSpecifiers typeSpecifierSeq() {
    DeclSpecifiers typeSpecifiers;
    TR(EX(typeSpecifier), typeSpecifiers);
    while (BT(EX(typeSpecifier), typeSpecifiers)) {
    }
    typeSpecifiers.finalize();
    return typeSpecifiers;
  }

  void aliasDeclaration() override {
    expect(KW_USING);
    auto name = expectIdentifier();
    expect(OP_ASS);
    auto type = TR(EX(typeId));
    expect(OP_SEMICOLON);

    curNamespace()->addTypedef(name, type);
  }

  void functionDefinition() override {
    DeclSpecifiers declSpecifiers = TR(EX(declSpecifierSeq));

    auto frame = translationUnit_->saveFrame();
    UDeclarator declarator = TR(EX(declarator), declSpecifiers);
    translationUnit_->restoreFrame(frame);

    TR(EX(functionBody));
    auto id = declarator->getId();
    if (declSpecifiers.isConstExpr()) {
      Throw("constexpr cannot be used for function definition: {}", 
            id.getName());
    }

    auto type = declarator->getType();
    if (!type->isFunction()) {
      Throw("Expect function-type in function-definition; got {}", *type);
    }

    auto target = getTargetNamespace(id);
    bool requirePriorDeclaration = id.isQualified();
    target->addFunction(id.unqualified,
                        static_pointer_cast<FunctionType>(type),
                        requirePriorDeclaration,
                        true, /* isDef */
                        declSpecifiers);
  }

  void functionBody() {
    expect(OP_LBRACE);
    expect(OP_RBRACE);
  }
};

} }

#undef EX
