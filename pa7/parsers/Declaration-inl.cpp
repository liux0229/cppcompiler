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

    frame_->openNamespace(
              curNamespace()->addNamespace(name, unnamed, isInline));

    TR(EX(namespaceBody));

    expect(OP_RBRACE);
    frame_->closeNamespace();
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

  Namespace::SNamespaceMember namespaceName(Namespace* root) {
    auto name = expectIdentifier();
    Namespace::SNamespaceMember ns;
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
      return frame_->getGlobalNamespace();
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

  Namespace::SNamespaceMember qualifiedNamespaceSpecifier() {
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
};

} }

#undef EX
