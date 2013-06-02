#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cctype>
#include <string>

#include "Utf8Encoder.h"
#include "Decoders.h"
#include "Tokenizer.h"
#include "PreprocessingToken.h"
#include "common.h"

namespace compiler {

using namespace std;

class PPTokenizer
{
public:
  template<typename T>
  void init()
  {
    decoders_.push_back(make_unique<T>());
  }

	PPTokenizer()
	{
    init<Utf8Decoder>();
    init<LineSplicer>();
    init<TrigraphDecoder>();
    init<UniversalCharNameDecoder>();
    init<CommentDecoder>();

    int n = decoders_.size();
    for (int i = 0; i < n - 1; i++) {
      decoders_[i]->sendTo([this, i](int x) {
          /* cout << format("decoder {} produced `{}`\n", i, char(x)); */
          if ((tokenizer_.insideRawString() && 
               decoders_[i + 1]->turnOffForRawString()) ||
              (tokenizer_.insideQuotedLiteral() &&
               decoders_[i + 1]->turnOffForQuotedLiteral())) {
            receivedChar(x);
          } else {
            decoders_[i + 1]->put(x);
          }
      });
    }

    using namespace std::placeholders;
    decoders_.back()->sendTo(bind(&PPTokenizer::receivedChar, this, _1));
    tokenizer_.sendTo(bind(&PPTokenizer::receivedToken, this, _1));
  }

  void process(int c) {
    if (c == EndOfFile) {
      // TODO: handle eliminated newline preceded by a newline
      // and a file ending with '\' (shouldn't add a second newline)
      if (pCh_ != -1 && pCh_ != '\n') {
        decoders_.front()->put('\n');
      }
    }
    decoders_.front()->put(c);
  }

  void receivedChar(int c) {
    pCh_ = c;
    tokenizer_.put(c);
  }

  bool isLiteral(const PPToken& token) const {
    return pToken_.type == PPTokenType::CharacterLiteral ||
           pToken_.type == PPTokenType::StringLiteral;
  }

  PPTokenType getUserDefined(PPTokenType type) const {
    return type == PPTokenType::CharacterLiteral ?
              PPTokenType::UserDefinedCharacterLiteral :
              PPTokenType::UserDefinedStringLiteral;
  }

  bool canMergeIntoUserDefined(const PPToken& token) const {
    bool isCharOrString = isLiteral(token);
    bool isIdentifier = token.type == PPTokenType::Identifier;

    return isCharOrString && isIdentifier;
  }

  void receivedToken(const PPToken& token) {
  if (canMergeIntoUserDefined(token)) {
      vector<int> data = pToken_.data;
      data.insert(data.end(), token.data.begin(), token.data.end());
      pToken_ = PPToken(getUserDefined(pToken_.type), move(data));
    } else {
      if (isLiteral(pToken_)) {
        printToken(pToken_);
      }
      pToken_ = token;
    }
    if (!isLiteral(pToken_)) {
      printToken(pToken_);
    }
  }

  void printToken(const PPToken& token) {
    if (token.type == PPTokenType::Eof) {
      cout << token.typeName() << endl;
    } else {
      string encoded = Utf8Encoder::encode(token.data);
      cout << format("{} {} {}", token.typeName(), encoded.size(), encoded) 
           << endl;
    }
  }

private:
  vector<unique_ptr<Decoder>> decoders_;
  Tokenizer tokenizer_;
  int pCh_ { -1 };
  PPToken pToken_;
};

}

int main()
{
  using namespace std;
  using namespace compiler;
	try
	{
    ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

		PPTokenizer tokenizer;

		for (char c : input)
		{
			unsigned char code_unit = c;
			tokenizer.process(code_unit);
		}

		tokenizer.process(EndOfFile);
	}
	catch (exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		return EXIT_FAILURE;
	}
}
