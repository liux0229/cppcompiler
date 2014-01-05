#include "Declarator.h"
#include "Expression.h"
#include <type_traits>

#define LOG() cout << format("[{}] index={} [{}]\n", \
                             __FUNCTION__, index_, cur().toStr());

#define expect(type) expectSimpleFromFunc(type, __FUNCTION__)
#define expectIdentifier() expectIdentifierFromFunc(__FUNCTION__)
#define expectLiteral() expectLiteralFromFunc(__FUNCTION__)
#define BAD_EXPECT(msg) complainExpect(msg, __FUNCTION__)

namespace compiler {

namespace SemanticParserImp {

using namespace std;

struct Base {
  static vector<UToken> emptyTokens_;

  // To be used by intermediate sub-modules
  Base() : tokens_(emptyTokens_) {
  }

  Base(const vector<UToken>& tokens, const ParserOption& option)
    : tokens_(tokens),
      option_(option) { 
    translationUnit_ = make_unique<TranslationUnit>();
  }

  struct ParserState {
    ParserState(size_t i)
      : index(i) {}
    size_t index;
  };

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

  // We rarely need to look ahead 2 chars
  // but in certain cases we do
  const PostToken& next() const {
    CHECK(index_ + 1 < tokens_.size());
    return *tokens_[index_ + 1];
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

  bool nextIsSimple(ETokenType type) const {
    if (!next().isSimple()) {
      return false;
    }
    return getSimpleTokenType(next()) == type;
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

  bool tryGetIdentifier(string& identifier) {
    if (!isIdentifier()) {
      return false;
    }
    identifier = getAdv()->toSimpleStr();
    return true;
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

  // traced call - f returns non-void
  template<typename F, typename... Args>
  auto TR(const char* name, F f, Args&&... args) ->
       typename
       enable_if<
         !is_void<decltype(f(forward<Args>(args)...))>::value,
         decltype(f(forward<Args>(args)...))>
       ::type {
    Trace trace(option_.isTrace, 
                name, 
                bind(&Base::cur, this), 
                traceDepth_);
    auto ret = f(forward<Args>(args)...);
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
         is_void<decltype(f(forward<Args>(args)...))>::value,
         bool>
       ::type {
    Trace trace(option_.isTrace, 
                name, 
                bind(&Base::cur, this), 
                traceDepth_);
    f(forward<Args>(args)...);
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

  Namespace* curNamespace() const {
    return translationUnit_->curNamespace();
  }

  /* ================================
   *  Interfaces between sub-modules
   * ================================
   */
  virtual void declaration() = 0;
  virtual void functionDefinition() = 0;
  virtual void simpleDeclaration() = 0;
  virtual void aliasDeclaration() = 0;
  virtual size_t constantExpression() = 0;
  virtual UExpression expression() = 0;
  virtual Namespace* nestedNameSpecifier() = 0;
  virtual UId idExpression() = 0;

  const vector<UToken>& tokens_;
  size_t index_ { 0 };

  ParserOption option_;
  int traceDepth_ { 0 };

  UTranslationUnit translationUnit_;
};

vector<UToken> Base::emptyTokens_;

} }
