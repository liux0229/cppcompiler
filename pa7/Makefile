CFLAGS=-g -Wall
CPPFLAGS=-g -Wall -std=gnu++11
CC=ccache gcc
CXX=ccache g++

PPDecoders=Utf8Decoder.o TrigraphDecoder.o UniversalCharNameDecoder.o
PPFSMs=PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o
PPTokenLib=$(PPDecoders) $(PPFSMs) Utf8Utils.o Utf8Encoder.o PreprocessingToken.o Tokenizer.o PPTokenizer.o
PPTokenOBJs=pptoken.o $(PPTokenLib)

PostTokenizers=CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o
PostTokenLib=PostTokenizer.o Utf16Encoder.o $(PostTokenizers)
PostTokenOBJs=posttoken.o $(PostTokenLib) $(PPTokenLib)

CtrlExprOBJs=CtrlExprEval.o ctrlexpr.o $(PostTokenLib) $(PPTokenLib)

PPDirectiveLib=BuildEnv.o PredefinedMacros.o SourceReader.o CtrlExprEval.o MacroProcessor.o PPDirective.o Preprocessor.o

MacroOBJs=macro.o $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

PreProcOBJs=preproc.o $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

ParserLib=Parser.o
RecogOBJs=recog.o $(ParserLib) $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

NsDeclLib=Driver.o SemanticParser.o Namespace.o Type.o DeclSpecifiers.o
NsDeclOBJs=nsdecl.o $(NsDeclLib) $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

all: clean pptoken posttoken ctrlexpr macro preproc recog nsdecl

pptoken: $(PPTokenOBJs)
	$(CXX) -g -std=gnu++11 -Wall -o pptoken $(PPTokenOBJs)

posttoken: $(PostTokenOBJs)
	$(CXX) -g -std=gnu++11 -Wall -o posttoken $(PostTokenOBJs)

ctrlexpr: $(CtrlExprOBJs)
	$(CXX) -g -std=gnu++11 -Wall -o ctrlexpr $(CtrlExprOBJs)

macro: $(MacroOBJs)
	$(CXX) -g -std=gnu++11 -Wall -o macro $(MacroOBJs)

preproc: $(PreProcOBJs)
	$(CXX) -g -std=gnu++11 -Wall -o preproc $(PreProcOBJs)

recog: $(RecogOBJs)
	$(CXX) -g -std=gnu++11 -Wall -o recog $(RecogOBJs)

nsdecl: $(NsDeclOBJs)
	$(CXX) -g -std=gnu++11 -Wall -o nsdecl $(NsDeclOBJs)

clean:
	rm -f *.o pptoken posttoken ctrlexpr macro preproc recog nsdecl

# test nsdecl application
test: clean nsdecl
	rm -f tests
	ln -s tests-pa7 tests
	scripts/run_all_tests.pl nsdecl my tests
	scripts/compare_results.pl ref my tests

# test recog application
test6: clean recog
	rm -f tests
	ln -s tests-pa6 tests
	scripts/run_all_tests.pl recog my tests
	scripts/compare_results.pl ref my tests

# test preproc application
test5: clean preproc
	rm -f tests
	ln -s tests-pa5 tests
	scripts/run_all_tests.pl preproc my tests
	scripts/compare_results.pl ref my tests

# test macro application
test4: clean macro
	scripts/run_all_tests.pl macro my tests-pa4
	scripts/compare_results.pl ref my tests-pa4

# test ctrlexpr application
test3: clean ctrlexpr
	scripts/run_all_tests.pl ctrlexpr my tests-pa3
	scripts/compare_results.pl ref my tests-pa3

# test posttoken application
test2: clean posttoken
	scripts/run_all_tests.pl posttoken my tests-pa2
	scripts/compare_results.pl ref my tests-pa2

# test pptoken application
test1: clean pptoken
	scripts/run_all_tests.pl pptoken my tests-pa1
	scripts/compare_results.pl ref my tests-pa1

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl ctrlexpr-ref ref
