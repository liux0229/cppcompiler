CFLAGS=-g -Wall
CPPFLAGS=-g -Wall -std=gnu++11
CC=ccache gcc
CXX=ccache g++

PPTokenOBJs=pptoken.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o
PostTokenOBJs=posttoken.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o PostTokenizer.o CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o Utf16Encoder.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o

all: clean pptoken posttoken

pptoken: $(PPTokenOBJs)
	g++ -g -std=gnu++11 -Wall -o pptoken $(PPTokenOBJs)

posttoken: $(PostTokenOBJs)
	g++ -g -std=gnu++11 -Wall -o posttoken $(PostTokenOBJs)

clean:
	rm -f *.o pptoken posttoken

# test posttoken application
test: all
	scripts/run_all_tests.pl posttoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl posttoken-ref ref

