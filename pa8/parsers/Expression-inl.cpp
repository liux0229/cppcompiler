#define EX(func) #func, make_delegate(&Expressions::func, this)

namespace compiler {

namespace SemanticParserImp {

struct Expressions : virtual Base {

  SLiteralExpression constantExpression() override {
    auto expr = TR(EX(expression));
    if (!expr->isConstant()) {
      Throw("expect constant-expression: {}", *expr);
    }
    return expr->toConstant();
  }

  SExpression expression() override {
    SConstantValue literal;
    if (tryAdvSimple(OP_LPAREN)) {
      return TR(EX(expression));
      expect(OP_RPAREN);
    } else if (tryAdvSimple(KW_TRUE)) {
      literal = ConstantValue::createFundalmentalValue(FT_BOOL, true);
    } else if (tryAdvSimple(KW_FALSE)) {
      literal = ConstantValue::createFundalmentalValue(FT_BOOL, false);
    } else if (tryAdvSimple(KW_NULLPTR)) {
      literal = ConstantValue::createFundalmentalValue(FT_NULLPTR_T, nullptr);
    } else if (auto value = tryGetLiteral()) {
      literal = value->toConstantValue();
    }
    if (literal) {
      return make_shared<LiteralExpression>(literal);
    }

    auto id = TR(EXB(idExpression));
    Namespace::MemberSet members;
    if (id->isQualified()) {
      members = id->ns->qualifiedLookup(id->unqualified);
    } else {
      members = curNamespace()->unqualifiedLookup(id->unqualified);
    }
    if (members.empty()) {
      Throw("{} is not declared", id->getName());
    } else if (members.size() > 1) {
      Throw("cannot resolve {}: {}", id->getName(), members);
    }

    return make_shared<IdExpression>(*members.begin());
  }

};

} }

#undef EX
