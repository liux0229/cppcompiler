#define EX(func) #func, make_delegate(&Declaration::func, this)

namespace compiler {

namespace SemanticParserImp {

struct Declaration : virtual Base {
  void declaration() override {
    BT(EX(emptyDeclaration)) ||
    BT(EX(namespaceDefinition)) ||
    BT(EX(usingDirective)) ||
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
    auto name = expectIdentifier();
    Namespace* used;
    if (ns) {
      used = ns->lookupNamespace(name, true);
    } else {
      used = curNamespace()->lookupNamespace(name, false);
    }
    if (!used) {
      Throw("using-directive ill-formed: {}::{}", ns->getName(), name);
    }
    curNamespace()->addUsingDirective(used);
    expect(OP_SEMICOLON);
  }

  Namespace* nestedNameSpecifier() {
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
      auto ns = curNamespace()->lookupNamespace(name, false);
      if (!ns) {
        Throw("Expect namespace-name; got {}", name);
      }
      return ns;
    }
  }

  Namespace* nestedNameSpecifierSuffix(Namespace* root) {
    auto name = expectIdentifier();
    expect(OP_COLON2);
    auto ns = root->lookupNamespace(name, true);
    if (!ns) {
      Throw("{}::{} is not a namespace", root->getName(), name);
    } 
    return ns;
  }

#if 0
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
};

} }

#undef EX
