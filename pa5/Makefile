CFLAGS=-g -Wall
CPPFLAGS=-g -Wall -std=gnu++11
CC=ccache gcc
CXX=ccache g++

PPTokenOBJs=pptoken.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o
PostTokenOBJs=posttoken.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o PostTokenizer.o CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o Utf16Encoder.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o
CtrlExprOBJs=CtrlExprEval.o ctrlexpr.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o PostTokenizer.o CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o Utf16Encoder.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o
MacroOBJs=SourceReader.o CtrlExprEval.o MacroProcessor.o PPDirective.o macro.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o PostTokenizer.o CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o Utf16Encoder.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o
PreProcOBJs=SourceReader.o Preprocessor.o CtrlExprEval.o MacroProcessor.o PPDirective.o preproc.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o PostTokenizer.o CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o Utf16Encoder.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o

all: clean pptoken posttoken ctrlexpr macro preproc

pptoken: $(PPTokenOBJs)
	g++ -g -std=gnu++11 -Wall -o pptoken $(PPTokenOBJs)

posttoken: $(PostTokenOBJs)
	g++ -g -std=gnu++11 -Wall -o posttoken $(PostTokenOBJs)

ctrlexpr: $(CtrlExprOBJs)
	g++ -g -std=gnu++11 -Wall -o ctrlexpr $(CtrlExprOBJs)

macro: $(MacroOBJs)
	g++ -g -std=gnu++11 -Wall -o macro $(MacroOBJs)

preproc: $(PreProcOBJs)
	g++ -g -std=gnu++11 -Wall -o preproc $(PreProcOBJs)

clean:
	rm -f *.o pptoken posttoken ctrlexpr macro preproc

# test macro application
test: all
	scripts/run_all_tests.pl preproc my tests
	scripts/compare_results.pl ref my tests

# test macro application
test4: all
	scripts/run_all_tests.pl macro my tests-pa4
	scripts/compare_results.pl ref my tests-pa4

# test ctrlexpr application
test3: all
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
