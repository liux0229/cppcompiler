#define EX(func) #func, make_delegate(&Declaration::func, this)

namespace compiler {

namespace SemanticParserImp {

struct Declaration : virtual Base {
  void declaration() override {
    BT(EX(emptyDeclaration)) ||
    BT(EX(namespaceDefinition)) ||
    BT(EX(namespaceAliasDefinition)) ||
    BT(EX(usingDirective)) ||
    BT(EX(usingDeclaration)) ||
    BT(EXB(aliasDeclaration)) ||
    BT(EXB(functionDefinition)) ||
    BT(EX(staticAssertDeclaration)) ||
    TR(EXB(simpleDeclaration));
  }

  void emptyDeclaration() {
    expect(OP_SEMICOLON);
  }

  void namespaceDefinition() {
    bool isInline = false;
    if (isSimple(KW_INLINE) && nextIsSimple(KW_NAMESPACE)) {
      isInline = true;
      adv();
    }
    expect(KW_NAMESPACE);
    string name;
    bool unnamed = !tryGetIdentifier(name);
    expect(OP_LBRACE);

    translationUnit_->openNamespace(
                       curNamespace()->addNamespace(name, unnamed, isInline));

    TR(EX(namespaceBody));

    expect(OP_RBRACE);
    translationUnit_->closeNamespace();
  }

  void namespaceBody() {
    while (BT(EX(declaration))) {
    }
  }

  void usingDirective() {
    expect(KW_USING);
    expect(KW_NAMESPACE);
    auto ns = BT(EX(nestedNameSpecifier));
    auto used = TR(EX(namespaceName), ns);
    curNamespace()->addUsingDirective(used->ns);
    expect(OP_SEMICOLON);
  }

  SNamespaceMember namespaceName(Namespace* root) {
    auto name = expectIdentifier();
    SNamespaceMember ns;
    if (root) {
      ns = root->lookupNamespace(name, true);
    } else {
      ns = curNamespace()->lookupNamespace(name, false);
    }
    if (!ns) {
      Throw("expect namespace-name; got: {}{}", 
            root ? root->getName() + "::" : string{},  
            name);
    }
    return ns;
  }

  void usingDeclaration() {
    expect(KW_USING);
    auto ns = TR(EX(nestedNameSpecifierRoot));
    auto name = TR(EX(unqualifiedId));
    expect(OP_SEMICOLON);

    auto members = ns->qualifiedLookup(name);
    curNamespace()->addUsingDeclaration(members);
  }

  Namespace* nestedNameSpecifier() override {
    auto ns = TR(EX(nestedNameSpecifierRoot));
    while (auto n = BT(EX(nestedNameSpecifierSuffix), ns)) {
      ns = n;
    }
    return ns;
  }

  Namespace* nestedNameSpecifierRoot() {
    if (tryAdvSimple(OP_COLON2)) {
      return translationUnit_->getGlobalNamespace();
    } else {
      auto name = expectIdentifier();
      expect(OP_COLON2);
      auto n = curNamespace()->lookupNamespace(name, false);
      if (!n) {
        Throw("Expect namespace-name; got {}", name);
      }
      return n->ns;
    }
  }

  Namespace* nestedNameSpecifierSuffix(Namespace* root) {
    auto name = expectIdentifier();
    expect(OP_COLON2);
    auto n = root->lookupNamespace(name, true);
    if (!n) {
      Throw("{}::{} is not a namespace", root->getName(), name);
    } 
    return n->ns;
  }

  void namespaceAliasDefinition() {
    expect(KW_NAMESPACE);
    auto name = expectIdentifier();
    expect(OP_ASS);
    auto ns = TR(EX(qualifiedNamespaceSpecifier));
    expect(OP_SEMICOLON);

    curNamespace()->addNamespaceAlias(name, ns);
  }

  SNamespaceMember qualifiedNamespaceSpecifier() {
    auto ns = BT(EX(nestedNameSpecifier));
    return TR(EX(namespaceName), ns);
  }

  UId idExpression() override {
    UId id = BT(EX(qualifiedId));
    if (id) {
      return id;
    } else {
      return make_unique<Id>(TR(EX(unqualifiedId)));
    }
  }

  string unqualifiedId() {
    return expectIdentifier();
  }

  UId qualifiedId() {
    auto ns = TR(EXB(nestedNameSpecifier));
    return make_unique<Id>(TR(EX(unqualifiedId)), ns);
  }

  void staticAssertDeclaration() {
    expect(KW_STATIC_ASSERT);
    expect(OP_LPAREN);
    auto literal = TR(EXB(constantExpression));
    expect(OP_COMMA);
    auto message = expectLiteral();
    expect(OP_RPAREN);
    expect(OP_SEMICOLON);

    // TODO: abstract this; and we are using the rule for '=' (copy assignment)
    // currently
    auto expr = literal->assignableTo(make_shared<FundalmentalType>(FT_BOOL));
    if (!expr) {
      Throw("{} cannot be contextually converted to bool", *literal);
    }
    auto boolLiteral = expr->toConstant();
    auto fund = dynamic_cast<FundalmentalValueBase*>(boolLiteral->value.get());
    CHECK(fund);
    
    if (fund->isZero()) {
      // directly terminate the program until we have better error reporting
      // TODO: better message output
      //       only-accept string-literal
      cerr << format("static-assert failed: {}", message->source) 
           << endl;
      exit(1);
    }
  }
};

} }

#undef EX
