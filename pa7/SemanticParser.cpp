#include "SemanticParser.h"
#include "NameUtility.h"

#include "Declarator.h"
#include "DeclSpecifiers.h"

#include <memory>
#include <vector>
#include <functional>
#include <sstream>
#include <map>
#include <type_traits>

#define LOG() cout << format("[{}] index={} [{}]\n", \
                             __FUNCTION__, index_, cur().toStr());

#define expect(type) expectSimpleFromFunc(type, __FUNCTION__)
#define expectIdentifier() expectIdentifierFromFunc(__FUNCTION__)
#define expectLiteral() expectLiteralFromFunc(__FUNCTION__)
#define BAD_EXPECT(msg) complainExpect(msg, __FUNCTION__)

#define EX(func) #func, &ParserImp::func

#define zeroOrMore(func) zeroOrMoreInternal(c, BTF(func));
#define zeroOrOne(func) zeroOrOneInternal(c, BTF(func));
#define oneOrMore(func) oneOrMoreInternal(c, TRF(func), BTF(func));

#define getAST(type) get(ASTType::type, move(c))

namespace compiler {

using namespace std;

// TODO: this function can be optimized by using a single ostringstream
// as the "environment"
string ASTNode::toStr(string indent, bool collapse) const {
  const char* indentInc = "|  ";
  ostringstream oss;
  if (isTerminal) {
    oss << indent;
    if (type != ASTType::Terminal) {
      oss << getASTTypeName(type) << ": ";
    }
    oss << token->toSimpleStr();
  } else {
    // if this node has a single child and collapse is enabled,
    // do not print the enclosing node and don't indent
    if (collapse && children.size() == 1) {
      oss << children.front()->toStr(indent, collapse);
    } else {
      oss << indent << getASTTypeName(type) << ":";
      for (auto& child : children) {
        oss << "\n" << child->toStr(indent + indentInc, collapse);
      }
    }
  }
  return oss.str();
}

class ParserImp
{
public:
  ParserImp(const vector<UToken>& tokens, const ParserOption& option)
    : tokens_(tokens),
      option_(option) { }
  void process() {
    TR(EX(translationUnit));
  }

private:
  typedef vector<AST> VAST;

  void translationUnit() {
    while (!isEof() && BT(true, EX(simpleDeclaration))) {
    }
    if (!isEof()) {
      BAD_EXPECT("<eof>");
    }
  }
#if 0
  AST translationUnit() {
    VAST c;
    zeroOrMore(declaration);
    // don't reduce EOF to allow the tracing facility to continue read it
    return getAST(TranslationUnit);
  }
#endif

  /* =====================
   *  constant expression
   * =====================
   */
  size_t constantExpression() {
    return expectLiteral()->toSigned64();
  }

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
  void simpleDeclaration() {
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
      size = TR(EX(constantExpression));
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

  /* ===================
   *  utility functions
   * ===================
   */
  struct ParserState {
    ParserState(size_t i)
      : index(i) {}
    size_t index;
  };

  void adv() { 
    if (option_.isTrace) {
      for (int i = 0; i < traceDepth_; ++i) {
        cout << Trace::padding();
      }
      cout << format("=== MATCH [{}]\n", cur().toStr());
    }

    ++index_; 
  }

  const PostToken* get() const {
    return &cur();
  }

  const PostToken* getAdv() {
    auto r = get();
    adv();
    return r;
  }

  const PostTokenSimple* getAdvSimple() {
    return static_cast<const PostTokenSimple*>(getAdv());
  }

  const PostToken& cur() const {
    return *tokens_[index_];
  }

  void reset(ParserState&& state) { 
    index_ = state.index;
  }

  bool isSimple(ETokenType type) const {
    if (!cur().isSimple()) {
      return false;
    }
    return getSimpleTokenType(cur()) == type;
  }

  bool isSimple(const vector<ETokenType>& types) const {
    for (ETokenType type : types) {
      if (isSimple(type)) {
        return true;
      }
    }
    return false;
  }

  const PostTokenSimple* tryAdvSimple(ETokenType type) {
    if (!isSimple(type)) {
      return nullptr;
    }
    return getAdvSimple();
  }

  void complainExpect(string&& expected, const char* func) const {
    Throw("[{}] expect {}; got: {}", 
          func,
          move(expected),
          cur().toStr());
  }

  const PostTokenSimple*
  expectSimpleFromFunc(ETokenType type, const char* func) {
    if (!isSimple(type)) {
      complainExpect(getSimpleTokenTypeName(type), func);
    }
    return getAdvSimple();
  }

  bool isIdentifier() const {
    return cur().isIdentifier();
  }

  string expectIdentifierFromFunc(const char* func) {
    if (!isIdentifier()) {
      complainExpect("identifier", func);
    }
    return getAdv()->toSimpleStr();
  }

  bool isLiteral() const {
    return cur().isLiteral();
  }

  const PostTokenLiteralBase* expectLiteralFromFunc(const char* func) {
    if (!isLiteral()) {
      complainExpect("literal", func);
    }
    return static_cast<const PostTokenLiteralBase*>(getAdv());
  }

  bool isEof() const {
    return cur().isEof();
  }

#if 0
  // Note: it is possible to use the follow set when FIRST does not match to
  // efficiently rule out the e-derivation - this might be helpful in emitting
  // useful error messages
  // @parseSep - return nullptr to indicate a non-match; throw to indicate parse
  //             failure
  AST conditionalRepeat(ASTType type, 
                        function<AST ()> subParser,
                        function<AST ()> parseSep) {
    VAST c;
    c.push_back(subParser());
    while (AST sep = parseSep()) {
      c.push_back(move(sep));
      c.push_back(subParser());
    }
    return get(type, move(c));
  }
  AST conditionalRepeat(ASTType type, 
                        function<AST ()> subParser,
                        ASTType sepType,
                        const vector<ETokenType>& seps) {
    return conditionalRepeat(
              type,
              subParser,
              [this, sepType, &seps]() -> AST {
                if (isSimple(seps)) {
                  return getAdv(sepType);  
                } else {
                  return nullptr;
                }
              });
  }
  AST conditionalRepeat(ASTType type, 
                        function<AST ()> subParser, 
                        ETokenType sep) {
    return conditionalRepeat(type, 
                             subParser, 
                             ASTType::Terminal, 
                             vector<ETokenType>{sep});
  }
#endif

  // TODO: this assumes that subParser is wrapped inside BTF
  void zeroOrMoreInternal(VAST& c, function<AST ()> subParser) {
    while (AST node = subParser()) {
      c.push_back(move(node));
    }
  }

  void zeroOrOneInternal(VAST& c, function<AST ()> subParser) {
    if (AST node = subParser()) {
      c.push_back(move(node));
    }
  }

  void oneOrMoreInternal(VAST& c, 
                         function<AST ()> first, 
                         function<AST ()> rest) {
    c.push_back(first());
    while (AST node = rest()) {
      c.push_back(move(node));
    }
  }

  /* ===========================
   *  traced call functionality
   * ===========================
   */
  class Trace {
  public:
    static constexpr const char* padding() {
      return "  ";
    }
    Trace(bool isTrace,
          string&& name, 
          function<const PostToken& ()> curTokenGetter,
          int& traceDepth)
      : isTrace_(isTrace),
        name_(move(name)),
        curTokenGetter_(curTokenGetter),
        traceDepth_(traceDepth)  {
      if (isTrace_) {
        tracePadding();
        cout << format("--> {} [{}]\n", name_, curTokenGetter_().toStr());
        ++traceDepth_;
      } 
    }
    ~Trace() {
      if (isTrace_) {
        --traceDepth_;
        tracePadding();
        cout << format("<-- {} {} [{}]\n", 
                       name_, 
                       ok_ ? "OK" : "BAD",
                       curTokenGetter_().toStr());
      }
    }
    void success() { ok_ = true; }
  private:
    void tracePadding() const {
      for (int i = 0; i < traceDepth_; ++i) {
        cout << padding();
      }
    }

    bool isTrace_;
    string name_;
    function<const PostToken& ()> curTokenGetter_;
    int& traceDepth_;
    bool ok_ { false };
  };
  
  // traced call - f returns non-void
  template<typename F, typename... Args>
  auto TR(const char* name, F f, Args&&... args) ->
       typename
       enable_if<
         !is_void<decltype(((*this).*f)(forward<Args>(args)...))>::value,
         decltype(((*this).*f)(forward<Args>(args)...))>
       ::type {
    Trace trace(option_.isTrace, 
                name, 
                bind(&ParserImp::cur, this), 
                traceDepth_);
    auto ret = ((*this).*f)(forward<Args>(args)...);
    trace.success();
    return ret;
  }

  // traced call - f returns void
  // transform f into a function which returns true on the success path,
  // which makes the backtrack implementation easier
  template<typename F, typename... Args>
  auto TR(const char* name, F f, Args&&... args) ->
       typename
       enable_if<
         is_void<decltype(((*this).*f)(forward<Args>(args)...))>::value,
         bool>
       ::type {
    Trace trace(option_.isTrace, 
                name, 
                bind(&ParserImp::cur, this), 
                traceDepth_);
    ((*this).*f)(forward<Args>(args)...);
    trace.success();
    return true;
  }

  // backtrack
  template<typename F, typename... Args>
  auto BT(bool reportError, const char* name, F f, Args&&... args) ->
       decltype(TR(name, f, forward<Args>(args)...)) {
    using Ret = decltype(TR(name, f, forward<Args>(args)...));
    ParserState state { index_ };
    try {
      return TR(name, f, forward<Args>(args)...);
    } catch (const CompilerException& e) {
      if (reportError) {
        cerr << "ERROR: " << e.what() << endl;
      }
      reset(move(state));
      return Ret{};
    }
  }

  template<typename F, typename... Args>
  auto BT(const char* name, F f, Args&&... args) ->
       decltype(TR(name, f, forward<Args>(args)...)) {
    return BT(false, name, f, forward<Args>(args)...);
  }

  const vector<UToken>& tokens_;
  size_t index_ { 0 };

  ParserOption option_;
  int traceDepth_ { 0 };
};

AST Parser::process()
{
  ParserImp(tokens_, option_).process();
  return nullptr;
}

}
