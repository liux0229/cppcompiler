#include "common.h"
#include "PostTokenizer.h"
#include "PostTokenUtils.h"
#include "PostProcessingToken.h"

namespace compiler {

using namespace std;

void PostTokenizer::handleSimpleOrIdentifier(const PPToken& token)
{
  string x = token.dataStrU8();
  auto it = StringToTokenTypeMap.find(x);
  if (it != StringToTokenTypeMap.end()) {
    writer_.put(PostTokenSimple(x, it->second));
  } else if (token.type == PPTokenType::Identifier) {
    writer_.put(PostTokenIdentifier(x));
  } else {
    writer_.put(PostTokenInvalid(x));
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
      case PPTokenType::PPNumber:
        if (!floatLiteralPT_.put(token) && 
            !intLiteralPT_.put(token)) {
          Throw("Bad pp-num: {}", token.dataStrU8());
        }
        break;
      case PPTokenType::WhitespaceSequence:
      case PPTokenType::NewLine:
        break;
      case PPTokenType::HeaderName:
        writer_.put(PostTokenInvalid(token.dataStrU8()));
        break;    
      case PPTokenType::NonWhitespaceChar:
        writer_.put(PostTokenInvalid(token.dataStrU8()));
        break;
      case PPTokenType::Eof:
        writer_.put(PostTokenEof());
        break;
      default:
        CHECK(false);
        break;
    }
  } catch (const exception& e) {
    cerr << format("ERROR: {}", e.what()) << std::endl;
    writer_.put(PostTokenInvalid(token.dataStrU8()));
  }
}
  
}
