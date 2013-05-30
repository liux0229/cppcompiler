#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "Utf8Decoder.h"
#include "Utf8Encoder.h"
#include "format.h"
#include "StateMachine.h"
#include "StateMachines.h"
#include "PreprocessingToken.h"
#include "common.h"
#include "LineSplicer.h"
#include "TrigraphDecoder.h"
#include "Tokenizer.h"

using namespace std;

// TODO: remove this
#include "IPPTokenStream.h"
#include "DebugPPTokenStream.h"

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

// See C++ standard 2.11 Identifiers and Appendix/Annex E.1
const vector<pair<int, int>> AnnexE1_Allowed_RangesSorted =
{
	{0xA8,0xA8},
	{0xAA,0xAA},
	{0xAD,0xAD},
	{0xAF,0xAF},
	{0xB2,0xB5},
	{0xB7,0xBA},
	{0xBC,0xBE},
	{0xC0,0xD6},
	{0xD8,0xF6},
	{0xF8,0xFF},
	{0x100,0x167F},
	{0x1681,0x180D},
	{0x180F,0x1FFF},
	{0x200B,0x200D},
	{0x202A,0x202E},
	{0x203F,0x2040},
	{0x2054,0x2054},
	{0x2060,0x206F},
	{0x2070,0x218F},
	{0x2460,0x24FF},
	{0x2776,0x2793},
	{0x2C00,0x2DFF},
	{0x2E80,0x2FFF},
	{0x3004,0x3007},
	{0x3021,0x302F},
	{0x3031,0x303F},
	{0x3040,0xD7FF},
	{0xF900,0xFD3D},
	{0xFD40,0xFDCF},
	{0xFDF0,0xFE44},
	{0xFE47,0xFFFD},
	{0x10000,0x1FFFD},
	{0x20000,0x2FFFD},
	{0x30000,0x3FFFD},
	{0x40000,0x4FFFD},
	{0x50000,0x5FFFD},
	{0x60000,0x6FFFD},
	{0x70000,0x7FFFD},
	{0x80000,0x8FFFD},
	{0x90000,0x9FFFD},
	{0xA0000,0xAFFFD},
	{0xB0000,0xBFFFD},
	{0xC0000,0xCFFFD},
	{0xD0000,0xDFFFD},
	{0xE0000,0xEFFFD}
};

// See C++ standard 2.11 Identifiers and Appendix/Annex E.2
const vector<pair<int, int>> AnnexE2_DisallowedInitially_RangesSorted =
{
	{0x300,0x36F},
	{0x1DC0,0x1DFF},
	{0x20D0,0x20FF},
	{0xFE20,0xFE2F}
};

// See C++ standard 2.13 Operators and punctuators
const unordered_set<string> Digraph_IdentifierLike_Operators =
{
	"new", "delete", "and", "and_eq", "bitand",
	"bitor", "compl", "not", "not_eq", "or",
	"or_eq", "xor", "xor_eq"
};

// See `simple-escape-sequence` grammar
const unordered_set<int> SimpleEscapeSequence_CodePoints =
{
	'\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
};

void writeToken(const PPToken& token)
{
  cout << format("{} {} {}", token.typeName(), token.data.size(), token.data) 
       << endl;
}

class PPTokenizer
{
public:
	PPTokenizer(IPPTokenStream& output)
		: output_(output)
	{
    using namespace std::placeholders;
    utf8Decoder_.sendTo(bind(&LineSplicer::put, &lineSplicer_, _1));
    lineSplicer_.sendTo(bind(&TrigraphDecoder::put, &trigraphDecoder_, _1));
    trigraphDecoder_.sendTo(bind(&Tokenizer::put, &tokenizer_, _1));
    tokenizer_.sendTo(bind(&PPTokenizer::received, this, _1));
  }

#if 0
	void process(int c)
	{
		// TODO:  Your code goes here.

		// 1. do translation features
		// 2. tokenize resulting stream
		// 3. call an output.emit_* function for each token.

    // c = utf8Decoder.put(c);
    // c = trigraphDecoder.put(c);
    Utf8Decode(c);

		if (c == EndOfFile)
		{
      utf8Decoder_.validate();
      if (currentFsm_ != PPTokenType::NotInitialized) {
        // TODO: handle an empty file case
        if (previousCh_ != '\n') {
          current().put('\n');
        }
        writeToken(current().get());

        currentFsm_ = PPTokenType::Eof;
        current().put(c);
        writeToken(current().get());
      }
			output_.emit_eof();
		} else {
      bool accepted = utf8Decoder_.put(c);
      if (accepted) {
        int x = utf8Decoder_.get();
        cout << format("raw={} hex={}", 
                       Utf8Encoder::encode(x),
                       utf8Decoder_.getStr())
             << endl;
      }

      // TIP: Reference implementation is about 1000 lines of code.
      // It is a state machine with about 50 states, most of which
      // are simple transitions of the operators.
    }
	}
#endif
  void process(int c) {
    if (c == EndOfFile) {
      if (previous_ != '\n') {
        utf8Decoder_.put('\n');
      }
    }
    utf8Decoder_.put(c);
  }

  void received(int c) {
    previous_ = c;
  }

  bool insideRawString() const {
    return false;
  }

private:
#if 0
  StateMachine& current() const {
    return *stateMachines_[static_cast<int>(currentFsm_)];
  }
#endif

	IPPTokenStream& output_;
  Utf8Decoder utf8Decoder_;
  LineSplicer lineSplicer_;
  TrigraphDecoder trigraphDecoder_;
  Tokenizer tokenizer_;
#if 0
  unique_ptr<StateMachine>
    stateMachines_[static_cast<int>(PPTokenType::Total)] {
      nullptr,
      make_unique<NewLineFSM>(),
      make_unique<EofFSM>()
    };
#endif
  int previous_ { -1 };
  PPTokenType currentFsm_ { PPTokenType::NotInitialized };
};

int main()
{
	try
	{
		ostringstream oss;
		oss << cin.rdbuf();

		string input = oss.str();

		DebugPPTokenStream output;

		PPTokenizer tokenizer(output);

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

