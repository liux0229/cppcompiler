CFLAGS=-g -Wall
CPPFLAGS=-g -Wall -std=gnu++11
CC=ccache gcc
CXX=ccache g++

PPTokenOBJs=pptoken.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o
PostTokenOBJs=posttoken.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o PostTokenizer.o CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o Utf16Encoder.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o
CtrlExprOBJs=CtrlExprEval.o ctrlexpr.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o PostTokenizer.o CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o Utf16Encoder.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o

all: clean pptoken posttoken ctrlexpr

pptoken: $(PPTokenOBJs)
	g++ -g -std=gnu++11 -Wall -o pptoken $(PPTokenOBJs)

posttoken: $(PostTokenOBJs)
	g++ -g -std=gnu++11 -Wall -o posttoken $(PostTokenOBJs)

ctrlexpr: $(CtrlExprOBJs)
	g++ -g -std=gnu++11 -Wall -o ctrlexpr $(CtrlExprOBJs)

clean:
	rm -f *.o pptoken posttoken ctrlexpr

# test ctrlexpr application
test: all
	scripts/run_all_tests.pl ctrlexpr my tests-pa3
	scripts/compare_results.pl ref my tests-pa3

# test posttoken application
test2: all
	scripts/run_all_tests.pl posttoken my tests-pa2
	scripts/compare_results.pl ref my tests-pa2

# test pptoken application
test1: all
	scripts/run_all_tests.pl pptoken my tests-pa1
	scripts/compare_results.pl ref my tests-pa1

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl ctrlexpr-ref ref
