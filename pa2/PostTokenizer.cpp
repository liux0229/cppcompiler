#include "common.h"
#include "PostTokenizer.h"
#include "PostTokenUtils.h"

#if 0
	// example usage:

	output.emit_invalid("foo");
	output.emit_simple("auto", KW_AUTO);

	u16string bar = u"bar";
	output.emit_literal_array("u\"bar\"", bar.size()+1, FT_CHAR16_T, bar.data(), bar.size() * 2 + 2);

	output.emit_user_defined_literal_integer("123_ud1", "ud1", "123");
#endif

namespace compiler {

using namespace std;

void PostTokenizer::handleSimpleOrIdentifier(const PPToken& token)
{
  string x = token.dataStrU8();
  auto it = StringToTokenTypeMap.find(x);
  if (it != StringToTokenTypeMap.end()) {
    writer_.emit_simple(x, it->second);
  } else if (token.type == PPTokenType::Identifier) {
    writer_.emit_identifier(x);
  } else {
    writer_.emit_invalid(x);
  }
}

void PostTokenizer::put(const PPToken& token)
{
  // printToken(token);
  try {
    if (token.type != PPTokenType::StringLiteral &&
        token.type != PPTokenType::UserDefinedStringLiteral &&
        token.type != PPTokenType::NewLine &&
        token.type != PPTokenType::WhitespaceSequence) {
      strLiteralPT_.terminate();
    }
    switch (token.type) {
      case PPTokenType::Identifier:
      case PPTokenType::PPOpOrPunc:
        handleSimpleOrIdentifier(token); 
        break;
      case PPTokenType::CharacterLiteral:
      case PPTokenType::UserDefinedCharacterLiteral:
        charLiteralPT_.put(token);
        break;
      case PPTokenType::StringLiteral:
      case PPTokenType::UserDefinedStringLiteral:
        strLiteralPT_.put(token);
        break;
      case PPTokenType::WhitespaceSequence:
      case PPTokenType::NewLine:
        break;
      case PPTokenType::HeaderName:
        writer_.emit_invalid(token.dataStrU8());
        break;    
      case PPTokenType::NonWhitespaceChar:
        writer_.emit_invalid(token.dataStrU8());
        break;
      case PPTokenType::Eof:
        printToken(token);
        break;
      default:
        printToken(token);
        break;
    }
  } catch (const exception& e) {
    cerr << format("ERROR: {}", e.what()) << std::endl;
    writer_.emit_invalid(token.dataStrU8());
  }
}
  
}
