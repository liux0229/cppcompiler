#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cctype>

#include "Utf8Encoder.h"
#include "Decoders.h"
#include "Tokenizer.h"
#include "PreprocessingToken.h"
#include "common.h"

using namespace std;

using namespace compiler;

// Translation features you need to implement:
// - utf8 decoder
// - utf8 encoder
// - universal-character-name decoder
// - trigraphs
// - line splicing
// - newline at eof
// - comment striping (can be part of whitespace-sequence)

// given hex digit character c, return its value
int HexCharToValue(int c)
{
	switch (c)
	{
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'A': return 10;
	case 'a': return 10;
	case 'B': return 11;
	case 'b': return 11;
	case 'C': return 12;
	case 'c': return 12;
	case 'D': return 13;
	case 'd': return 13;
	case 'E': return 14;
	case 'e': return 14;
	case 'F': return 15;
	case 'f': return 15;
	default: throw logic_error("HexCharToValue of nonhex char");
	}
}

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
          decoders_[i + 1]->put(x);
      });
    }

    using namespace std::placeholders;
    decoders_.back()->sendTo(bind(&PPTokenizer::receivedChar, this, _1));
    tokenizer_.sendTo(bind(&PPTokenizer::receivedToken, this, _1));
  }

  void process(int c) {
    if (c == EndOfFile) {
      // TODO: handle empty file and eliminated newline preceded by a newline
      // and a file ending with '\' (shouldn't add a second newline)
      if (pCh_ != '\n') {
        decoders_.front()->put('\n');
      }
    }
    decoders_.front()->put(c);
  }

  void receivedChar(int c) {
    pCh_ = c;
    tokenizer_.put(c);
  }

  bool canMergeIntoUserDefined(const PPToken& token) const {
    bool isCharOrString = pToken_.type == PPTokenType::CharacterLiteral ||
                          pToken_.type == PPTokenType::StringLiteral;
    bool isIdentifier = 
      token.type == PPTokenType::Identifier ||
      // a little hacky
      (token.type == PPTokenType::PPOpOrPunc && isalpha(token.data[0]));

    return isCharOrString && isIdentifier;
  }

  void receivedToken(const PPToken& token) {
  if (canMergeIntoUserDefined(token)) {
      vector<int> data = pToken_.data;
      data.insert(data.end(), token.data.begin(), token.data.end());
      pToken_ = PPToken(PPTokenType::UserDefinedCharacterLiteral, move(data));
    } else {
      pToken_ = token;
    }
    printToken(pToken_);
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

  bool insideRawString() const {
    return false;
  }

private:
  vector<unique_ptr<Decoder>> decoders_;
  Tokenizer tokenizer_;
  int pCh_ { -1 };
  PPToken pToken_;
};

int main()
{
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

