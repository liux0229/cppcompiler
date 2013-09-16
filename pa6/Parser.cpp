// TODO: 
// 1. use multiple inheritance for composition
// circular reference is a problem
// 2. What happens if I use contexpr instead of static const inside a function
// 4. We could build some simple format of error reporting
//    1) we could report the failure happens when try to parse a certain NT
//    2) Display the line and the relevant position
//       ERROR: expect OP_RPAREN; got: simple sizeof KW_SIZEOF
// 5. complete id-expression
#include "Parser.h"
#include "NameUtility.h"
#include <memory>
#include <vector>
#include <functional>
#include <sstream>
#include <map>

#define LOG() cout << format("[{}] index={} [{}]\n", \
                             __FUNCTION__, index_, cur().toStr());

#define expect(type) expectFromFunc(type, __FUNCTION__)
#define BAD_EXPECT(msg) complainExpect(msg, __FUNCTION__)

// traced call
#define TR(func) tracedCall(&ParserImp::func, #func)
#define TRF(func) ([this]() -> AST \
                   { return tracedCall(&ParserImp::func, #func); })
// backtracked call - can be later optimized away by using FIRST and FOLLOW
#define BT(func) backtrack(&ParserImp::func, #func)

#if 0
// shorthand
#define cpbe(type) c.push_back(expect(type))
#endif

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
  ParserImp(const vector<UToken>& tokens, bool isTrace)
    : tokens_(tokens),
      isTrace_(isTrace) { }
  AST process() {
    AST root = TR(constantExpression);
    cout << root->toStr(true /* collapse */) << endl;
    // root = constantExpression();
    // cout << root->toStr(false /* collapse */) << endl;
    return root;
  }

private:
  typedef vector<AST> VAST;

  /* =====================
   *  constant expression
   * =====================
   */
  AST constantExpression() {
    return TR(conditionalExpression);
  }

  AST conditionalExpression() {
    VAST c;
    c.push_back(TR(logicalOrExpression));
    return finishConditionalExpression(c);
  }

  AST finishConditionalExpression(VAST& c) {
    if (isSimple(OP_QMARK)) {
      c.push_back(expect(OP_QMARK));
      c.push_back(TR(expression));
      c.push_back(expect(OP_COLON));
      c.push_back(TR(assignmentExpression));
    }
    return get(ASTType::ConditionalExpression, move(c));
  }

  AST logicalOrExpression() {
    return conditionalRepeat(ASTType::LogicalOrExpression, 
                             TRF(logicalAndExpression), 
                             OP_LOR);
  }

  AST logicalAndExpression() {
    return conditionalRepeat(ASTType::LogicalAndExpression, 
                             TRF(inclusiveOrExpression), 
                             OP_LAND);
  }

  AST expression() {
    return conditionalRepeat(ASTType::Expression,
                             TRF(assignmentExpression), 
                             OP_COMMA);
  }

  AST assignmentExpression() {
    if (isSimple(KW_THROW)) {
      return TR(throwExpression);
    }
    VAST c;
    c.push_back(TR(logicalOrExpression));
    if (isAssignmentOperator()) {
      c.push_back(getAdv(ASTType::AssignmentOperator));
      c.push_back(TR(initializerClause));
      return get(ASTType::AssignmentExpression, move(c));
    } else {
      // the assignmentExpression is swallowed, which is fine
      return finishConditionalExpression(c);
    }
  }

  bool isAssignmentOperator() const {
    return isSimple({
              OP_ASS,
              OP_STARASS,
              OP_DIVASS,
              OP_MODASS,
              OP_PLUSASS,
              OP_MINUSASS,
              OP_RSHIFTASS,
              OP_LSHIFTASS,
              OP_BANDASS,
              OP_XORASS,
              OP_BORASS
           });
  }

  AST inclusiveOrExpression() {
    return conditionalRepeat(ASTType::InclusiveOrExpression, 
                             TRF(exclusiveOrExpression), 
                             OP_BOR);
  }

  AST exclusiveOrExpression() {
    return conditionalRepeat(ASTType::ExclusiveOrExpression, 
                             TRF(andExpression), 
                             OP_XOR);
  }

  AST andExpression() {
    return conditionalRepeat(ASTType::AndExpression, 
                             TRF(equalityExpression), 
                             OP_AMP);
  }

  AST equalityExpression() {
    return conditionalRepeat(
              ASTType::EqualityExpression,
              TRF(relationalExpression), 
              ASTType::EqualityOperator,
              { OP_EQ, OP_NE });
  }

  AST relationalExpression() {
    return conditionalRepeat(
              ASTType::RelationalExpression,
              TRF(shiftExpression),
              ASTType::RelationalOperator,
              { OP_LT, OP_GT, OP_LE, OP_GE });
  }

  AST shiftExpression() {
    return conditionalRepeat(
              ASTType::ShiftExpression,
              TRF(additiveExpression),
              TRF(shiftOperator));
  }

  AST shiftOperator() {
    if (isSimple(OP_LSHIFT)) {
      return getAdv(ASTType::ShiftOperator);
    } else if (isSimple(OP_RSHIFT_1)) {
      VAST c;
      c.push_back(getAdv());
      c.push_back(expect(OP_RSHIFT_2));
      return get(ASTType::ShiftOperator, move(c));
    } else {
      // this will make tracing less regular, but it's fine
      return nullptr;
    }
  }

  AST additiveExpression() {
    return conditionalRepeat(
              ASTType::AdditiveExpression,
              TRF(mulplicativeExpression),
              ASTType::AdditiveOperator,
              { OP_PLUS, OP_MINUS });
  }

  AST mulplicativeExpression() {
    return conditionalRepeat(
              ASTType::MulplicativeExpression,
              TRF(pmExpression),
              ASTType::MulplicativeOperator,
              { OP_STAR, OP_DIV, OP_MOD });
  }

  AST pmExpression() {
    return conditionalRepeat(
              ASTType::PmExpression,
              TRF(castExpression),
              ASTType::PmOperator,
              { OP_DOTSTAR, OP_ARROWSTAR });
  }

  AST castExpression() {
    if (isSimple(OP_LPAREN)) {
      VAST c;
      // TODO: consider possible ways to optimize this
      // the reason we use backtrack here is for the cases how
      // (Class * Class) is handled
      AST castOp = BT(castOperator);
      if (castOp) {
        c.push_back(move(castOp));
        c.push_back(TR(castExpression));
        return get(ASTType::CastExpression, move(c));
      } // else failed to parse castOperator
        // try to parse unaryExpression instead
    }

    return TR(unaryExpression);
  }

  AST typeIdInParen(ASTType type) {
    VAST c;
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(typeId));
    c.push_back(expect(OP_RPAREN));
    return get(type, move(c));
  }

  AST castOperator() {
    return typeIdInParen(ASTType::CastOperator);
  }

  AST unaryExpression() {
    VAST c;
    if (isSimple(KW_SIZEOF)) {
      c.push_back(getAdv());
      if (isSimple(OP_LPAREN)) {
        c.push_back(typeIdInParen(ASTType::TypeIdInParen));
      } else if (isSimple(OP_DOTS)) {
        c.push_back(getAdv());
        c.push_back(expect(OP_LPAREN));
        c.push_back(expectIdentifier());
        c.push_back(expect(OP_RPAREN));
      } else {
        c.push_back(TR(unaryExpression));
      }
      return get(ASTType::UnaryExpression, move(c));
    } if (isSimple(KW_ALIGNOF)) {
      c.push_back(getAdv());
      c.push_back(typeIdInParen(ASTType::TypeIdInParen));
      return get(ASTType::UnaryExpression, move(c));
    } else if (isUnaryOperator()) {
      // TODO: seems to be ambiguous with regard to 
      // id-expression (~class_name)
      // need to resolve this
      c.push_back(getAdv(ASTType::UnaryOperator));
      c.push_back(TR(castExpression));
      return get(ASTType::UnaryExpression, move(c));
    } else if (isSimple(KW_NOEXCEPT)) {
      return TR(noExceptExpression);
    } else if (AST e = TR(newOrDeleteExpression)) {
      return e;
    } else {
      return TR(postfixExpression);
    }
  }

  AST newOrDeleteExpression() {
    if (isSimple(OP_COLON2)) {
      AST colon2 = getAdv();
      if (isSimple(KW_NEW)) {
        return newExpression(move(colon2));
      } else if (isSimple(KW_DELETE)) {
        return deleteExpression(move(colon2));
      } else {
        // TODO: these kinds of messages expose internal implementations
        BAD_EXPECT("KW_NEW or KW_DELETE");
        return nullptr;
      }
    } else if (isSimple(KW_NEW)) {
      return newExpression(nullptr);
    } else if (isSimple(KW_DELETE)) {
      return deleteExpression(nullptr);
    } else {
      return nullptr;
    }
  }

  AST postfixExpression() {
    VAST c;
    c.push_back(TR(postfixRoot));
    return get(ASTType::PostfixExpression, move(c));
  }

  AST postfixRoot() {
    VAST c;
    c.push_back(TR(primaryExpression));
    return get(ASTType::PostfixRoot, move(c));
  }

  AST primaryExpression() {
    VAST c;
    if (isSimple(KW_TRUE)) {
      c.push_back(getAdv());
    } else if (isSimple(KW_FALSE)) {
      c.push_back(getAdv());
    } else if (isSimple(KW_NULLPTR)) {
      c.push_back(getAdv());
    } else if (isSimple(KW_THIS)) {
      c.push_back(getAdv());
    } else if (isLiteral()) {
      c.push_back(getAdv());
    } else if (isSimple(OP_LPAREN)) {
      c.push_back(getAdv());
      c.push_back(TR(expression));
      c.push_back(expect(OP_RPAREN));
    } else if (isSimple(OP_LSQUARE)) {
      c.push_back(TR(lambdaExpression));
    } else {
      c.push_back(TR(idExpression));
    }
    return get(ASTType::PrimaryExpression, move(c));
  }

  AST noExceptExpression() {
    VAST c;
    c.push_back(expect(KW_NOEXCEPT));
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(expression));
    c.push_back(expect(OP_RPAREN));
    return get(ASTType::NoExceptExpression, move(c));
  }

  bool isUnaryOperator() {
    return isSimple({
              OP_INC,
              OP_DEC,
              OP_STAR,
              OP_AMP,
              OP_PLUS,
              OP_MINUS,
              OP_LNOT,
              OP_COMPL
           });
  }

  /* =============
   *  declarators
   * =============
   */
  AST typeId() {
    throw CompilerException("typeId not implemented");
  }

  AST declaration() {
    return nullptr;
  }

  AST declarator() {
    return nullptr;
  }

  /* ===============
   *  id expression
   * ===============
   */
  AST idExpression() {
    // TODO: make the parsing more effective
    VAST c;
    auto node = BT(qualifiedId);
    if (!node) {
      node = TR(unqualifiedId);
    }
    c.push_back(move(node));
    return get(ASTType::IdExpression, move(c));
  }

  AST unqualifiedId() {
    VAST c;
    if (isIdentifier()) {
      c.push_back(getAdv(ASTType::Identifier));
    } else if (isSimple(KW_OPERATOR)) {
      // almost as efficient as checking FIRST, except for the exception catch
      AST node;
      (node = BT(literalOperatorId)) ||
      (node = BT(operatorFunctionId)) ||
      (node = TR(conversionFunctionId));
      c.push_back(move(node));
    } else if (isSimple(OP_COMPL)) {
      c.push_back(getAdv());
      if (isSimple(KW_DECLTYPE)) {
        c.push_back(TR(decltypeSpecifier));
      } else {
        c.push_back(TR(className));
      }
    }
    return get(ASTType::UnqualifiedId, move(c));
  }

  AST operatorFunctionId() {
    VAST c;
    c.push_back(expect(KW_OPERATOR));

    auto parseDouble = [this, &c](const vector<ETokenType>& m) -> bool {
      if (isSimple(m[0])) {
        c.push_back(getAdv());
        c.push_back(expect(m[1]));
        return true;
      } else {
        return false;
      }
    };

    // need to parse non-singles first to disambiguate
    if (isSimple({ KW_NEW, KW_DELETE })) {
      c.push_back(getAdv());
      parseDouble({ OP_LSQUARE, OP_RSQUARE });
    } else {
      bool ok = parseDouble({ OP_RSHIFT_1, OP_RSHIFT_2 }) ||
                parseDouble({ OP_LSQUARE, OP_RSQUARE }) ||
                parseDouble({ OP_LPAREN, OP_RPAREN });
      if (!ok) {
        static vector<ETokenType> singles { 
          OP_PLUS,
          OP_MINUS,
          OP_STAR,
          OP_DIV,
          OP_MOD,
          OP_XOR,
          OP_AMP,
          OP_BOR,
          OP_COMPL,
          OP_LNOT,
          OP_ASS,
          OP_LT,
          OP_GT,
          OP_PLUSASS,
          OP_MINUSASS,
          OP_STARASS,
          OP_DIVASS,
          OP_MODASS,
          OP_XORASS,
          OP_BANDASS,
          OP_BORASS,
          OP_LSHIFT,
          OP_RSHIFTASS ,
          OP_LSHIFTASS,
          OP_EQ,
          OP_NE,
          OP_LE,
          OP_GE,
          OP_LAND,
          OP_LOR,
          OP_INC,
          OP_DEC,
          OP_COMMA,
          OP_ARROWSTAR,
          OP_ARROW
        };
        if (isSimple(singles)) {
          c.push_back(getAdv());
        } else {
          BAD_EXPECT("operator");
        }
      }
    }

    return get(ASTType::OperatorFunctionId, move(c));
  }

  AST literalOperatorId() {
    VAST c;
    c.push_back(expect(KW_OPERATOR));
    if (!isEmptyStr()) {
      BAD_EXPECT("empty str");
    }
    c.push_back(getAdv());
    c.push_back(expectIdentifier());
    return get(ASTType::LiteralOperatorId, move(c));
  }

  AST conversionFunctionId() {
    VAST c;
    c.push_back(expect(KW_OPERATOR));
    c.push_back(TR(conversionTypeId));
    return get(ASTType::ConversionFunctionId, move(c));
  }

  AST conversionTypeId() {
    VAST c;
    c.push_back(TR(typeSpecifierSeq));
    AST node;
    while (node = BT(ptrOperator)) {
      c.push_back(move(node));
    }
    return get(ASTType::ConversionTypeId, move(c));
  }

  AST typeSpecifierSeq() {
    VAST c;
    c.push_back(TR(typeSpecifier));
    AST node;
    while (node = BT(typeSpecifier)) {
      c.push_back(move(node));
    }
    while (node = BT(attributeSpecifier)) {
      c.push_back(move(node));
    }
    return get(ASTType::TypeSpecifierSeq, move(c));
  }

  AST typeSpecifier() {
    AST node;
    (node = BT(enumSpecifier)) ||
    (node = BT(classSpecifier)) ||
    (node = TR(trailingTypeSpecifier));
    VAST c;
    c.push_back(move(node));
    return get(ASTType::TypeSpecifier, move(c));
  }

  AST enumSpecifier() {
    VAST c;
    c.push_back(TR(enumHead));
    c.push_back(expect(OP_LBRACE));
    if (!isSimple(OP_RBRACE)) {
      // reduce non-empty enumerator-list
      c.push_back(TR(enumeratorList));
      // optionally reduce an OP_COMMA
      if (isSimple(OP_COMMA)) {
        c.push_back(getAdv());
      }
    }
    c.push_back(expect(OP_RBRACE));
    return get(ASTType::EnumSpecifier, move(c));
  }

  AST enumHead() {
    VAST c;
    c.push_back(TR(enumKey));
    AST node;
    while (node = BT(attributeSpecifier)) {
      c.push_back(move(node));
    }
    // try to reduce a nested-name-specifier
    if (node = BT(nestedNameSpecifier)) {
      c.push_back(move(node));
      // must reduce an identifier
      c.push_back(expectIdentifier());
    } else {
      if (isIdentifier()) {
        c.push_back(getAdv(ASTType::Identifier));
      }
    }
    if (isSimple(OP_COLON)) {
      c.push_back(TR(enumBase));
    }
    return get(ASTType::EnumHead, move(c));
  }

  AST enumBase() {
    VAST c;
    c.push_back(expect(OP_COLON));
    c.push_back(TR(typeSpecifierSeq));
    return get(ASTType::EnumBase, move(c));
  }

  AST enumKey() {
    VAST c;
    c.push_back(expect(KW_ENUM));
    if (isSimple({ KW_CLASS, KW_STRUCT })) {
      c.push_back(getAdv());
    }
    return get(ASTType::EnumKey, move(c));
  }

  AST enumeratorList() {
    return conditionalRepeat(ASTType::EnumeratorList,
                             TRF(enumeratorDefinition),
                             OP_COMMA);
  }

  AST enumeratorDefinition() {
    VAST c;
    c.push_back(expectIdentifier());
    if (isSimple(OP_ASS)) {
      c.push_back(getAdv());
      c.push_back(TR(constantExpression));
    }
    return get(ASTType::EnumeratorDefinition, move(c));
  }

  AST nestedNameSpecifier() {
    VAST c;
    c.push_back(TR(nestedNameSpecifierRoot));
    // We can do an optimization to check
    // isSimple(KW_TEMPLATE) || isIdentifier()
    // Since nestedNameSpecifier's FOLLOW also contains the two tokens
    // we need to be prepared to backtrack
    // So strive for simplicity for now
    AST node;
    while (node = BT(nestedNameSpecifierSuffix)) {
      c.push_back(move(node));
    }
    return get(ASTType::NestedNameSpecifier, move(c));
  }

  AST nestedNameSpecifierRoot() {
    VAST c;
    if (isSimple(KW_DECLTYPE)) {
      c.push_back(TR(decltypeSpecifier));
    } else {
      if (isSimple(OP_COLON2)) {
        c.push_back(getAdv());
        if (isNamespaceName()) {
          c.push_back(TR(namespaceName));
        } else {
          c.push_back(TR(typeName));
        }
      }
    }
    c.push_back(expect(OP_COLON2));
    return get(ASTType::NestedNameSpecifierRoot, move(c));
  }

  AST nestedNameSpecifierSuffix() {
    VAST c;
    if (isIdentifier() && nextIsSimple(OP_COLON2)) {
      c.push_back(getAdv());
      c.push_back(getAdv());
    } else {
      if (isSimple(KW_TEMPLATE)) {
        c.push_back(getAdv());
      }
      c.push_back(TR(simpleTemplateId));
      c.push_back(expect(OP_COLON2));
    }
    return get(ASTType::NestedNameSpecifierSuffix, move(c));
  }

  AST trailingTypeSpecifier() {
    return nullptr;
  }

  AST attributeSpecifier() {
    return nullptr;
  }

  AST ptrOperator() {
    return nullptr;
  }

  /* =================
   *  class specifier
   * =================
   */
  AST classSpecifier() {
    VAST c;
    c.push_back(TR(classHead));
    c.push_back(expect(OP_LBRACE));
    while (!isSimple(OP_RBRACE)) {
      c.push_back(TR(memberSpecification));
    }
    c.push_back(getAdv());
    return get(ASTType::ClassSpecifier, move(c));
  }

  AST classHead() {
    VAST c;
    c.push_back(TR(classKey));
    AST node;
    // TODO: we could have used FIRST as the FIRST set is very small
    while (node = BT(attributeSpecifier)) {
      c.push_back(move(node));
    }
    if (node = BT(classHeadName)) {
      c.push_back(move(node));
      // class-virt-specifier?
      if (isStFinal()) {
        c.push_back(getAdv(ASTType::StFinal));
      }
    }
    if (isSimple(OP_COLON)) {
      c.push_back(TR(baseClause));
    }
    return get(ASTType::ClassHead, move(c));
  }

  AST classKey() {
    if (!isSimple({KW_CLASS, KW_STRUCT, KW_UNION})) {
      BAD_EXPECT("class key");
    }
    return getAdv(ASTType::ClassKey);
  }

  AST classHeadName() {
    return nullptr;
  }

  AST baseClause() {
    VAST c;
    c.push_back(expect(OP_COLON));
    c.push_back(TR(baseSpecifierList));
    return get(ASTType::BaseClause, move(c));
  }

  AST baseSpecifierList() {
    return conditionalRepeat(ASTType::BaseSpecifierList,
                             TRF(baseSpecifierDots),
                             OP_COMMA);
  }

  AST baseSpecifierDots() {
    VAST c;
    c.push_back(TR(baseSpecifer));
    if (isSimple(OP_DOTS)) {
      c.push_back(getAdv());
    }
    return get(ASTType::BaseSpecifierDots, move(c));
  }

  AST baseSpecifer() {
    VAST c;
    AST node;
    while (node = BT(attributeSpecifier)) {
      c.push_back(move(node));
    }
    if (isSimple(KW_VIRTUAL)) {
      c.push_back(getAdv());
      if (node = BT(accessSpecifier)) {
        c.push_back(move(node));
      }
    } else if (node = BT(accessSpecifier)) {
      c.push_back(move(node));
      if (isSimple(KW_VIRTUAL)) {
        c.push_back(getAdv());
      }
    }
    c.push_back(TR(baseTypeSpecifier));
    return get(ASTType::BaseSpecifer, move(c));
  }

  AST baseTypeSpecifier() {
    VAST c;
    c.push_back(TR(classOrDecltype));
    return get(ASTType::BaseTypeSpecifier, move(c));
  }

  AST classOrDecltype() {
    VAST c;
    AST node;
    if (node = BT(decltypeSpecifier)) {
      c.push_back(move(node));
    } else {
      if (node = BT(nestedNameSpecifier)) {
        c.push_back(move(node));
      }
      c.push_back(TR(className));
    }
    return get(ASTType::ClassOrDecltype, move(c));
  }

  AST accessSpecifier() {
    if (!isSimple({KW_PRIVATE, KW_PROTECTED, KW_PUBLIC})) {
      BAD_EXPECT("access specifier");
    }
    return getAdv(ASTType::AccessSpecifier);
  }

  AST memberSpecification() {
    VAST c;
    AST node;
    if (node = BT(accessSpecifier)) {
      c.push_back(move(node));
      c.push_back(expect(OP_COLON));
    } else {
      c.push_back(TR(memberDeclaration));
    }
    return get(ASTType::MemberSpecification, move(c));
  }

  AST memberDeclaration() {
    VAST c;
    AST node;
    bool ok = (node = BT(usingDeclaration)) ||
              (node = BT(staticAssertDeclaration)) ||
              (node = BT(templateDeclaration)) ||
              (node = BT(aliasDeclaration));
    if (ok) {
      c.push_back(move(node));
    } else {
      if (node = BT(functionDefinition)) {
        c.push_back(move(node));
        if (isSimple(OP_SEMICOLON)) {
          c.push_back(getAdv());
        }
      } else {
        while (node = BT(attributeSpecifier)) {
          c.push_back(move(node));
        } 
        c.push_back(TR(declSpecifierSeq));
        if (node = BT(memberDeclaratorList)) {
          c.push_back(move(node));
        }
        c.push_back(expect(OP_SEMICOLON));
      } 
    }
    return get(ASTType::MemberDeclaration, move(c));
  }

  AST declSpecifierSeq() {
    return nullptr;
  }

  AST memberDeclaratorList() {
    return conditionalRepeat(ASTType::MemberDeclaratorList,
                             TRF(memberDeclarator),
                             OP_COMMA);
  }

  AST memberDeclarator() {
    VAST c;
    AST node;
    if (node = BT(declarator)) {
      c.push_back(move(node));
      // TODO: use FOLLOW set to prune the search
      if (node = BT(braceOrEqualInitializer)) {
        c.push_back(move(node));
      } else {
        while (node = BT(virtSpecifier)) {
          c.push_back(move(node));
        }
        if (node = BT(pureSpecifier)) {
          c.push_back(move(node));
        }
      }
    } else {
      if (isIdentifier()) {
        c.push_back(getAdv(ASTType::Identifier));
      }
      while (node = attributeSpecifier()) {
        c.push_back(move(node));
      }
      c.push_back(expect(OP_COLON));
      c.push_back(TR(constantExpression));
    }
    return get(ASTType::MemberDeclarator, move(c));
  }

  AST braceOrEqualInitializer() {
    VAST c;
    if (isSimple(OP_ASS)) {
      c.push_back(getAdv());
      c.push_back(TR(initializerClause));
    } else {
      c.push_back(TR(bracedInitList));
    }
    return get(ASTType::BraceOrEqualInitializer, move(c));
  }

  AST pureSpecifier() {
    VAST c;
    c.push_back(expect(OP_ASS));
    c.push_back(expectZero());
    return get(ASTType::PureSpecifier, move(c));
  }

  AST functionDefinition() {
    VAST c;
    AST node;
    while (node = BT(attributeSpecifier)) {
      c.push_back(move(node));
    }
    c.push_back(TR(declSpecifierSeq));
    c.push_back(TR(declarator));
    if (node = BT(virtSpecifier)) {
      c.push_back(move(node));
    }
    c.push_back(TR(functionBody));
    return get(ASTType::FunctionDefinition, move(c));
  }

  AST functionBody() {
    VAST c;
    if (isSimple(OP_ASS)) {
      c.push_back(getAdv());
      if (!isSimple({KW_DEFAULT, KW_DELETE})) {
        BAD_EXPECT("default / delete");
      }
      c.push_back(expect(OP_SEMICOLON));
    } else if (isSimple(KW_TRY)) {
      c.push_back(TR(functionTryBlock));
    } else {
      if (isSimple(OP_COLON)) {
        c.push_back(TR(ctorInitializer));
      }
      c.push_back(TR(compoundStatement));
    }
    return get(ASTType::FunctionBody, move(c));
  }

  AST ctorInitializer() {
    VAST c;
    c.push_back(expect(OP_COLON));
    c.push_back(TR(memInitialierList));
    return get(ASTType::CtorInitializer, move(c));
  }

  // TODO: the same pattern appears more than once
  AST memInitialierList() {
    return conditionalRepeat(ASTType::MemInitialierList,
                             TRF(memInitialierDots),
                             OP_COMMA);
  }

  AST memInitialierDots() {
    VAST c;
    c.push_back(TR(memInitialier));
    if (isSimple(OP_DOTS)) {
      c.push_back(getAdv());
    }
    return get(ASTType::MemInitialierDots, move(c));
  }

  AST memInitialier() {
    VAST c;
    c.push_back(TR(memInitialierId));
    if (isSimple(OP_LPAREN)) {
      c.push_back(getAdv());
      AST node;
      if (node = BT(expressionList)) {
        c.push_back(move(node));
      }
      c.push_back(expect(OP_RPAREN));
    } else {
      c.push_back(TR(bracedInitList));
    }
    return get(ASTType::MemInitialier, move(c));
  }

  AST memInitialierId() {
    return nullptr;
  }

  AST expressionList() {
    return TR(initializerList);
  }

  AST compoundStatement() {
    return nullptr;
  }

  AST functionTryBlock() {
    return nullptr;
  }

  AST virtSpecifier() {
    if (!(isStFinal() || isStOverride())) {
      BAD_EXPECT("virt specifier");  
    }
    return getAdv(ASTType::VirtSpecifier);
  }

  AST usingDeclaration() {
    VAST c;
    c.push_back(expect(KW_USING));
    if (isSimple(OP_COLON2)) {
      c.push_back(getAdv());
    } else {
      if (isSimple(KW_TYPENAME)) {
        c.push_back(getAdv());
      }
      c.push_back(TR(nestedNameSpecifier));
    }
    c.push_back(TR(unqualifiedId));
    c.push_back(expect(OP_SEMICOLON));
    return get(ASTType::UsingDeclaration, move(c));
  }

  AST staticAssertDeclaration() {
    VAST c;
    c.push_back(expect(KW_STATIC_ASSERT));
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(constantExpression));
    c.push_back(expect(OP_COMMA));
    c.push_back(expectLiteral());
    c.push_back(expect(OP_RPAREN));
    c.push_back(expect(OP_SEMICOLON));
    return get(ASTType::StaticAssertDeclaration, move(c));
  }

  AST templateDeclaration() {
    VAST c;
    c.push_back(expect(KW_TEMPLATE));
    c.push_back(expect(OP_LT));
    c.push_back(TR(templateParameterList));
    c.push_back(TR(closeAngleBracket));
    c.push_back(TR(declaration));
    return get(ASTType::TemplateDeclaration, move(c));
  }

  AST templateParameterList() {
    return nullptr;
  }

  AST aliasDeclaration() {
    VAST c;
    c.push_back(expect(KW_USING));
    c.push_back(expectIdentifier());
    AST node;
    while (node = BT(attributeSpecifier)) {
      c.push_back(move(node));
    }
    c.push_back(expect(OP_ASS));
    c.push_back(TR(typeId));
    c.push_back(expect(OP_SEMICOLON));
    return get(ASTType::AliasDeclaration, move(c));
  } 

  AST className() { 
    VAST c;
    if (!isClassName()) {
      BAD_EXPECT("class name"); 
    }
    if (isTemplateName()) {
      c.push_back(TR(simpleTemplateId)); 
    } else {
      c.push_back(getAdv(ASTType::Identifier));
    }
    return get(ASTType::ClassName, move(c));
  }

  AST typeName() {
    // An important NT - give it a level
    VAST c;
    AST node;
    (node = BT(className)) ||
    (node = BT(enumName)) ||
    (node = BT(typedefName)) ||
    (node = TR(simpleTemplateId));
    c.push_back(move(node));
    return get(ASTType::TypeName, move(c));
  }

  AST namespaceName() {
    if (!isNamespaceName()) {
      BAD_EXPECT("namespace name");
    }
    // should be fine to not use the Identifier type
    return getAdv(ASTType::NamespaceName);
  }

  AST enumName() {
    if (!isEnumName()) {
      BAD_EXPECT("enum name");
    }
    return getAdv(ASTType::EnumName);
  }

  AST typedefName() {
    if (!isTypedefName()) {
      BAD_EXPECT("typedef name");
    }
    return getAdv(ASTType::TypedefName);
  }

  AST simpleTemplateId() {
    VAST c;
    c.push_back(expectTemplateName());
    c.push_back(expect(OP_LT));
    auto node = BT(templateArgumentList);
    if (node) {
      c.push_back(move(node));
    }
    c.push_back(TR(closeAngleBracket));
    return get(ASTType::SimpleTemplateId, move(c));
  }

  AST templateArgumentList() {
    return conditionalRepeat(ASTType::TemplateArgumentList,
                             TRF(templateArgumentDots),
                             OP_COMMA);
  }

  AST templateArgumentDots() {
    VAST c;
    c.push_back(templateArgument());
    if (isSimple(OP_DOTS)) {
      c.push_back(getAdv());
    }
    return get(ASTType::TemplateArgumentDots, move(c));
  }

  AST templateArgument() {
    VAST c;
    if (isSimple(KW_DECLTYPE) || isSimple(KW_OPERATOR) || isSimple(OP_COLON2) ||
        isSimple(OP_COMPL) || isIdentifier()) {
      c.push_back(TR(idExpression));
    } else {
      // Implement FIRST here
      auto node = BT(constantExpression);
      if (!node) {
        node = BT(typeId);
      }
      c.push_back(move(node));
    }
    return get(ASTType::TemplateArgument, move(c));
  }

  AST closeAngleBracket() {
    // note that this is handled in a special manner and we should never call
    // isSimple() from within this
    bool ok = !treatRAngleBracketAsOperator() && cur().isSimple();
    if (ok) {
      auto type = getSimpleTokenType(cur());
      ok = type == OP_GT || type == OP_RSHIFT_1 || type == OP_RSHIFT_2;
    }
    if (!ok) {
      BAD_EXPECT("right angle bracket");
    }
    VAST c;
    c.push_back(getAdv());
    // TODO: why cannot I use { getAdv() } as the 2nd argument
    return get(ASTType::CloseAngleBracket, move(c));
  }

  AST qualifiedId() {
    throw CompilerException("not implemented");
  }

  AST decltypeSpecifier() {
    VAST c;
    // TODO: here we follow the convention that a function should not assume
    // its caller has checked the initial sequence of tokens
    // This is not the most efficient implementation and is worth considering
    // consolidating in the future
    c.push_back(expect(KW_DECLTYPE));
    c.push_back(expect(OP_LPAREN));
    c.push_back(TR(expression));
    c.push_back(expect(OP_RPAREN));
    return get(ASTType::DecltypeSpecifier, move(c));
  }

  /* ===================
   *  lambda expression
   * ===================
   */
  AST lambdaExpression() {
    return nullptr;
  }

  /* ===============
   *  new or delete
   * ===============
   */
  AST newExpression(AST colon2) {
    return nullptr;
  }

  AST deleteExpression(AST colon2) {
    return nullptr;
  }

  /* ====================
   *  initializer clause
   * ====================
   */
  AST initializerClause() {
    VAST c;
    if (isSimple(OP_LBRACE)) {
      c.push_back(TR(bracedInitList));
    } else {
      c.push_back(TR(assignmentExpression));
    }
    // normally we want to swallow initializerClause,
    // but it's clearer to keep it in the parse tree
    return get(ASTType::InitializerClause, move(c));
  }

  AST bracedInitList() {
    VAST c;
    c.push_back(expect(OP_LBRACE));
    if (!isSimple(OP_LBRACE)) {
      c.push_back(TR(initializerList));
      if (isSimple(OP_COMMA)) {
        c.push_back(getAdv());
      }
    }
    c.push_back(expect(OP_RBRACE));
    return get(ASTType::BracedInitList, move(c));
  }

  AST initializerList() {
    VAST c;
    c.push_back(TR(initializerClauseDots));
    while (isSimple(OP_COMMA)) {
      // Here we need to look ahead 2 chars
      if (nextIsSimple(OP_RBRACE)) {
        // initializerClauseDots cannot match OP_RBRACE
        // FIRST(initializerClauseDots) !-] OP_RBRACE
        // but the upper level can, so stop matching now
        break;
      }
      // TODO; here is an example chance where we can use FIRST and/or
      // FOLLOW to prune
      c.push_back(getAdv());
      c.push_back(TR(initializerClauseDots));
    }
    return get(ASTType::InitializerList, move(c));
  }

  AST initializerClauseDots() {
    VAST c;
    c.push_back(TR(initializerClause));
    if (isSimple(OP_DOTS)) {
      c.push_back(getAdv());
    }
    return get(ASTType::InitializerClauseDots, move(c));
  }

  /* ==================
   *  throw expression
   * ==================
   */
  AST throwExpression() {
    return nullptr;
  }

  /* ===================
   *  utility functions
   * ===================
   */
  typedef AST (ParserImp::*SubParser)();
  AST get(ASTType type = ASTType::Terminal) const {
    return make_unique<ASTNode>(type, &cur());
  }
  AST getAdv(ASTType type = ASTType::Terminal) {
    auto r = get(type);
    adv();
    return r;
  }
  AST get(ASTType type, VAST&& c) const {
    return make_unique<ASTNode>(type, move(c));
  }

  const PostToken& cur() const {
    return *tokens_[index_];
  }
  size_t curPos() const {
    return index_;
  }
  void reset(size_t pos) { index_ = pos; }

  // We rarely need to look ahead 2 chars
  // but in certain cases we do
  const PostToken& next() const {
    CHECK(index_ + 1 < tokens_.size());
    return *tokens_[index_ + 1];
  }
  void adv() { 
    if (isTrace_) {
      for (int i = 0; i < traceDepth_; ++i) {
        cout << Trace::padding();
      }
      cout << format("=== MATCH [{}]\n", cur().toStr());
    }

    handleBrackets();

    ++index_; 
  }

  /* ==================
   *  handle brackets
   * ==================
   */
  // TODO: consider making this a separate class
  // Maintain nested levels of brackets
  void handleBrackets() {
    static map<ETokenType, ETokenType> mapping {
      { OP_RSQUARE, OP_LSQUARE },
      { OP_RPAREN, OP_LPAREN },
      { OP_RBRACE, OP_LBRACE }
    };
    if (isSimple({OP_LSQUARE, OP_LPAREN, OP_LBRACE, OP_LT})) {
      // starting a new nested level - always allowed
      brackets_.push_back(index_);
    } else if (isSimple({OP_RSQUARE, OP_RPAREN, OP_RBRACE})) {
      auto lhs = mapping[getSimpleTokenType(cur())];
      // our parsing shouldn't allow this to happen
      CHECK(!brackets_.empty() && 
            lhs == getSimpleTokenType(*tokens_[brackets_.back()]));
      brackets_.pop_back();
    } else if (isSimple({OP_GT, OP_RSHIFT_1, OP_RSHIFT_2})) {
      if (!brackets_.empty() && 
          getSimpleTokenType(*tokens_[brackets_.back()]) == OP_LT) {
        // this terminal has been treated as a closing bracket in the parsing
        // so 'close' the beginning bracket
        brackets_.pop_back();
      }
      // else this terminal was treated as an operator in the parsing
      // so ignore it for bracketing purposes
    }
  }
  bool treatRAngleBracketAsOperator() const {
    return brackets_.empty() || 
           getSimpleTokenType(*tokens_[brackets_.back()]) != OP_LT;
  }

  void complainExpect(string&& expected, const char* func) const {
    Throw("[{}] expect {}; got: {}", 
          func,
          move(expected),
          cur().toStr());
  }

  AST expectFromFunc(ETokenType type, const char* func) {
    if (!isSimple(type)) {
      complainExpect(getSimpleTokenTypeName(type), func);
    }
    return getAdv();
  }

  bool isIdentifier() const {
    return cur().isIdentifier();
  }
  bool isEmptyStr() const {
    return cur().isEmptyStr();
  }
  AST expectIdentifier() {
    if (!isIdentifier()) {
      BAD_EXPECT("identifier");
    }
    return getAdv(ASTType::Identifier);
  }

// TODO: this involves a copy
#define GEN_IS_NAME(func) \
  bool func() const {\
    return isIdentifier() && PA6::func(cur().toSimpleStr());\
  }
  GEN_IS_NAME(isClassName)
  GEN_IS_NAME(isTemplateName)
  GEN_IS_NAME(isTypedefName)
  GEN_IS_NAME(isEnumName)
  GEN_IS_NAME(isNamespaceName)
#undef GEN_IS_NAME

  AST expectTemplateName() {
    if (!isTemplateName()) {
      BAD_EXPECT("template name");
    }
    return getAdv(ASTType::Identifier);
  }

  bool isLiteral() const {
    return cur().isLiteral();
  }
  AST expectLiteral() {
    if (!isLiteral()) {
      BAD_EXPECT("literal");
    }
    return getAdv();
  }
  // TODO: move this to util
  ETokenType getSimpleTokenType(const PostToken& token) const {
    // can consider removing this check
    CHECK(token.isSimple());
    return static_cast<const PostTokenSimple&>(token).type;
  }
  bool isSimple(ETokenType type) const {
    if (!cur().isSimple()) {
      return false;
    }
  
    // here we assume the caller is expecting an operator
    // closing-angle-bracket is handled in a special manner
    // this is to avoid passing in an extra parameter
    if (type == OP_GT || type == OP_RSHIFT_1 || type == OP_RSHIFT_2) {
      // TODO: OP_RSHIFT_2 should not be necessary
      if (!treatRAngleBracketAsOperator()) {
        // if this check fails, never treat the current token as an operator
        // even if it's a r-bracket
        return false;
      }
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

  bool nextIsSimple(ETokenType type) const {
    if (!next().isSimple()) {
      return false;
    }
    return getSimpleTokenType(next()) == type;
  }

  AST expectZero() {
    if (!cur().isZero()) {
      BAD_EXPECT("0");
    }
    return getAdv(ASTType::StZero);
  }

  /* ====================
   * Contextual keywords
   * ====================
   */
  bool isStFinal() const {
    return isIdentifier() && cur().toSimpleStr() == "final";
  }
  bool isStOverride() const {
    return isIdentifier() && cur().toSimpleStr() == "override";
  }

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
  
  AST tracedCall(SubParser parser, const char* name) {
    Trace trace(isTrace_, name, bind(&ParserImp::cur, this), traceDepth_);
    AST root = CALL_MEM_FUNC(*this, parser)();
    trace.success();
    return root;
  }

  AST backtrack(SubParser parser, const char *name) {
    size_t curIndex = index_;
    try {
      return tracedCall(parser, name);
    } catch (const CompilerException&) {
      reset(curIndex);
      return nullptr;
    }
  }

  const vector<UToken>& tokens_;
  size_t index_ { 0 };

  bool isTrace_;
  int traceDepth_ { 0 };

  vector<size_t> brackets_;
};

AST Parser::process()
{
  return ParserImp(tokens_, isTrace_).process();
}

}
