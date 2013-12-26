#define EX(func) #func, make_delegate(&ConstantExpression::func, this)

namespace compiler {

namespace SemanticParserImp {

struct ConstantExpression : virtual Base {

  size_t constantExpression() override {
    return expectLiteral()->toSigned64();
  }

};

} }

#undef EX
