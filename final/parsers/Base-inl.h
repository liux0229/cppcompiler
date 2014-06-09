/*
 * Design notes:
 *
 * Semantics of disableBt()
 * When the parsing procedure detects a pattern of terminals (which match
 * a prefix of itself, that cannot match any other non-terminal, it can
 * optionally call disableBt() to signal that information to the parsing
 * framework, which can then disable backtracking on the corresponding path,
 * because, since no other NT can match the terminal prefix, if the parsing
 * procedure fails to parse the remaining terminals, no other NTs can parse
 * the terminal string.
 *
 * This mechanism not only speeds up parsing but also provides much better
 * error reporting.
 *
 * Note that the currently implemented mechanism does not restrict itself
 * to a one-level-down relationship only - a parsing procedure at level L might
 * have the intelligence to determine that a failure of the above type at level
 * L + x (x > 1) means the parsing at L must fail. The framework provides the
 * basic support to express that in the forms of sharing the same BtControl,
 * and it's not hard to extend the current mechanism to support this semantics
 * if the current implementation is not powerful enough.
 *
 * This mechanism is a generalized FIRST set mechanism, and thus FIRST
 * optimization can be implemented on top of this mechanism (e.g. by grouping
 * parsing procedures sharing the same prefix into a single parsing procedure).
 * However to automatic support every case of FIRST, we need an automatic FIRST
 * / FOLLOW generation mechanism.
 *
 * It's not hard to imagine we can also incorporate FOLLOW into this mechanism,
 * since FOLLOW is just a way of determining without backtracking the upper
 * level cannot be successful if we followed an empty reduction.
 *
 */
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

    // Note: postActionCall is abstracted out because VC++ cannot handle
    // member template functions which are differentiated by
    // an enable_if return type
    // This is probably a cleaner design anyways

    template<typename F, typename... Args>
    auto postActionCall(function<void()> postAction, F f, Args&&... args) ->
      typename
      enable_if<
      !is_void<decltype(f(forward<Args>(args)...))>::value,
      decltype(f(forward<Args>(args)...))>::type
    {
      auto ret = f(forward<Args>(args)...);
      postAction();
      return ret;
    }

    template<typename F, typename... Args>
    auto postActionCall(function<void()> postAction, F f, Args&&... args) ->
      typename
      enable_if<
      is_void<decltype(f(forward<Args>(args)...))>::value,
      bool>::type
    {
      f(forward<Args>(args)...);
      postAction();
      return true;
    }

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
        static const char* padding() {
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
        bool ok_{ false };
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

      // TODO: consistent naming (tryAdv vs. tryGet)
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

      const PostTokenLiteralBase* tryGetLiteral() {
        if (!isLiteral()) {
          return nullptr;
        }
        return static_cast<const PostTokenLiteralBase*>(getAdv());
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

      struct BtControl {
        bool disableBt{ false };
        bool reportError{ false };
      };

      // traced call
      template<typename F, typename... Args>
      auto TR(BtControl& btControl, const char* name, F f, Args&&... args) ->
        typename
        conditional<
        is_void<decltype(f(forward<Args>(args)...))>::value,
        bool,
        decltype(f(forward<Args>(args)...))
        >::type {
        BtControlGuard guard(this, btControl);
        Trace trace(option_.isTrace,
                    name,
                    bind(&Base::cur, this),
                    traceDepth_);
        function<void()> postAction = [&trace]() { trace.success(); };
        return postActionCall(postAction, f, forward<Args>(args)...);
      }

      // traced call - provide stack frame's BtControl

      // note: this was how the return type was specified
      // but that seemed to cause recursive evaluations from the VC++
      // (and prevented it from compiling)
      // However the new signature is probably cleaner anyways
      /*
         template<typename F, typename... Args>
         auto TR(const char* name, F f, Args&&... args) ->
         decltype(TR(*static_cast<BtControl*>(nullptr),
         name,
         f,
         forward<Args>(args)...));
         */
      template<typename F, typename... Args>
      auto TR(const char* name, F f, Args&&... args) ->
        typename
        conditional<
        is_void<decltype(f(forward<Args>(args)...))>::value,
        bool,
        decltype(f(forward<Args>(args)...))
        >::type
      {
        BtControl btControl;
        return TR(btControl, name, f, forward<Args>(args)...);
      }

      // backtrack
      template<typename F, typename... Args>
      auto BT(BtControl& btControl, const char* name, F f, Args&&... args) ->
        decltype(TR(btControl, name, f, forward<Args>(args)...)) {
        using Ret = decltype(TR(btControl, name, f, forward<Args>(args)...));
        ParserState state{ index_ };
        try {
          return TR(btControl, name, f, forward<Args>(args)...);
        }
        catch (const CompilerException& e) {
          if (btControl.reportError) {
            cerr << "ERROR: " << e.what() << endl;
          }

          if (btControl.disableBt) {
            throw;
          }

          reset(move(state));
          return Ret{};
        }
      }

      // backtrack - provide stack frame's BtControl
      template<typename F, typename... Args>
      auto BT(const char* name, F f, Args&&... args) ->
        decltype(TR(name, f, forward<Args>(args)...)) {
        BtControl btControl;
        return BT(btControl, name, f, forward<Args>(args)...);
      }

      // TODO: assess the usability of this
      template<typename F, typename... Args>
      auto BT(bool reportError, const char* name, F f, Args&&... args) ->
        decltype(TR(name, f, forward<Args>(args)...)) {
        BtControl btControl;
        btControl.reportError = true;
        return BT(btControl, name, f, forward<Args>(args)...);
      }

      void disableBt() {
        CHECK(!btControlStack_.empty());
        btControlStack_.back()->disableBt = true;
      }

      BtControl& curBtControl() const {
        CHECK(!btControlStack_.empty());
        return *btControlStack_.back();
      }

      void addBtControl(BtControl& btControl) {
        btControlStack_.push_back(&btControl);
      }

      void rmBtControl() {
        CHECK(!btControlStack_.empty());
        btControlStack_.pop_back();
      }

      struct BtControlGuard {
        BtControlGuard(Base* base, BtControl& btControl) : base_(base) {
          base_->addBtControl(btControl);
        }
        ~BtControlGuard() {
          base_->rmBtControl();
        }
        Base* base_;
      };

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
      virtual SLiteralExpression constantExpression() = 0;
      virtual SExpression expression() = 0;
      virtual Namespace* nestedNameSpecifier() = 0;
      virtual UId idExpression() = 0;

      const vector<UToken>& tokens_;
      size_t index_{ 0 };

      ParserOption option_;
      int traceDepth_{ 0 };

      vector<BtControl*> btControlStack_;

      UTranslationUnit translationUnit_;
    };

    vector<UToken> Base::emptyTokens_;

  }
}
