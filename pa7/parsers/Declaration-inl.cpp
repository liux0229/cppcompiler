#define EX(func) #func, make_delegate(&Declaration::func, this)

namespace compiler {

namespace SemanticParserImp {

struct Declaration : virtual Base {
  void declaration() override {
    BT(EX(emptyDeclaration)) ||
    BT(EX(namespaceDefinition)) ||
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
