CFLAGS=-g -Wall
CPPFLAGS=-g -Wall -std=gnu++11
CC=ccache gcc
CXX=ccache g++

OBJs=pptoken.o Utf8Decoder.o Utf8Utils.o Utf8Encoder.o PreprocessingToken.o TrigraphDecoder.o Tokenizer.o UniversalCharNameDecoder.o PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o PPTokenizer.o

all: pptoken

# build pptoken application
pptoken: clean $(OBJs)
	g++ -g -std=gnu++11 -Wall -o pptoken $(OBJs)

clean:
	rm -f *.o pptoken

format: format.h format-test.cpp Exceptions.h
	g++ -g -std=gnu++11 -Wall -o format format-test.cpp

# test pptoken application
test: all
	scripts/run_all_tests.pl pptoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl pptoken-ref ref

