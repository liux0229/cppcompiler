#define EX(func) #func, make_delegate(&Expression::func, this)

namespace compiler {

namespace SemanticParserImp {

struct Expression : virtual Base {

  size_t constantExpression() override {
    return expectLiteral()->toSigned64();
  }

  UExpression expression() override {
    return nullptr;
  }

};

} }

#undef EX
