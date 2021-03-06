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
    receiver_.put(PostTokenSimple(x, it->second));
  } else if (token.type == PPTokenType::Identifier) {
    receiver_.put(PostTokenIdentifier(x));
  } else {
    receiver_.put(PostTokenInvalid(x));
  }
}

void PostTokenizer::put(const PPToken& token)
{
  // printToken(token);
  try {
    if (token.type != PPTokenType::StringLiteral &&
        token.type != PPTokenType::UserDefinedStringLiteral &&
        (noStrCatForNewLine_ || token.type != PPTokenType::NewLine) &&
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
        break;
      case PPTokenType::NewLine:
        receiver_.put(PostTokenNewLine());
        break;
      case PPTokenType::HeaderName:
        receiver_.put(PostTokenInvalid(token.dataStrU8()));
        break;    
      case PPTokenType::NonWhitespaceChar:
        receiver_.put(PostTokenInvalid(token.dataStrU8()));
        break;
      case PPTokenType::Eof:
        receiver_.put(PostTokenEof());
        break;
      default:
        CHECK(false);
        break;
    }
  } catch (const exception& e) {
    cerr << format("ERROR: {}", e.what()) << std::endl;
    receiver_.put(PostTokenInvalid(token.dataStrU8()));
  }
}
  
}
