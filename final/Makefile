CFLAGS=-g -Wall
CPPFLAGS=-g -Wall -std=gnu++11 -I. -fsanitize=address
CC=ccache gcc
CXX=ccache g++

PPDecoders=Utf8Decoder.o TrigraphDecoder.o UniversalCharNameDecoder.o
PPFSMs=PPOpOrPuncFSM.o IdentifierFSM.o PPNumberFSM.o QuotedLiteralFSM.o RawStringLiteralFSM.o
PPTokenLib=$(PPDecoders) $(PPFSMs) Utf8Utils.o Utf8Encoder.o PreprocessingToken.o Tokenizer.o PPTokenizer.o
PPTokenOBJs=pptoken.o $(PPTokenLib)

PostTokenizers=CharLiteralPostTokenizer.o StringLiteralPostTokenizer.o FloatLiteralPostTokenizer.o IntegerLiteralPostTokenizer.o
PostTokenLib=PostTokenizer.o Utf16Encoder.o Type.o ConstantValue.o Expression.o Member.o Namespace.o DeclSpecifiers.o StorageClass.o $(PostTokenizers)
PostTokenOBJs=posttoken.o $(PostTokenLib) $(PPTokenLib)

CtrlExprOBJs=CtrlExprEval.o ctrlexpr.o $(PostTokenLib) $(PPTokenLib)

PPDirectiveLib=BuildEnv.o PredefinedMacros.o SourceReader.o CtrlExprEval.o MacroProcessor.o PPDirective.o Preprocessor.o

MacroOBJs=macro.o $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

PreProcOBJs=preproc.o $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

ParserLib=Parser.o
RecogOBJs=recog.o $(ParserLib) $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

NsDeclLib=Driver.o parsers/SemanticParser.o
NsDeclOBJs=nsdecl.o $(NsDeclLib) $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

NsInitLib=Linker.o
NsInitOBJs=nsinit.o $(NsInitLib) $(NsDeclLib) $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

Cy86Lib=Cy86Compiler.o Cy86Parser.o Cy86Instruction.o X86Instruction.o
Cy86OBJs=cy86.o $(Cy86Lib) $(PPDirectiveLib) $(PostTokenLib) $(PPTokenLib)

all: clean pptoken posttoken ctrlexpr macro preproc recog nsdecl nsinit

pptoken: $(PPTokenOBJs)
	$(CXX) $(CPPFLAGS) -o pptoken $(PPTokenOBJs)

posttoken: $(PostTokenOBJs)
	$(CXX) $(CPPFLAGS) -o posttoken $(PostTokenOBJs)

ctrlexpr: $(CtrlExprOBJs)
	$(CXX) $(CPPFLAGS) -o ctrlexpr $(CtrlExprOBJs)

macro: $(MacroOBJs)
	$(CXX) $(CPPFLAGS) -o macro $(MacroOBJs)

preproc: $(PreProcOBJs)
	$(CXX) $(CPPFLAGS) -o preproc $(PreProcOBJs)

recog: $(RecogOBJs)
	$(CXX) $(CPPFLAGS) -o recog $(RecogOBJs)

nsdecl: $(NsDeclOBJs)
	$(CXX) $(CPPFLAGS) -o nsdecl $(NsDeclOBJs)

nsinit: $(NsInitOBJs)
	$(CXX) $(CPPFLAGS) -o nsinit $(NsInitOBJs)

cy86: $(Cy86OBJs)
	$(CXX) $(CPPFLAGS) -o cy86 $(Cy86OBJs)

clean:
	rm -f *.o parsers/*.o pptoken posttoken ctrlexpr macro preproc recog nsdecl nsinit

tests: test9 test8 test7 test6 test5 test4 test3 test2 test1

test9: clean cy86
	rm -f tests
	ln -s tests-pa9 tests
	tests/scripts/run_all_tests.pl cy86 my
	tests/scripts/compare_results.pl ref my

test8: clean nsinit
	rm -f tests
	ln -s tests-pa8 tests
	tests/scripts/run_all_tests.pl nsinit my
	tests/scripts/compare_results.pl ref my

# test nsdecl application
test7: clean nsdecl
	rm -f tests
	ln -s tests-pa7 tests
	tests/scripts/run_all_tests.pl nsdecl my
	tests/scripts/compare_results.pl ref my

# test recog application
test6: clean recog
	rm -f tests
	ln -s tests-pa6 tests
	tests/scripts/run_all_tests.pl recog my
	tests/scripts/compare_results.pl ref my

# test preproc application
test5: clean preproc
	rm -f tests
	ln -s tests-pa5 tests
	tests/scripts/run_all_tests.pl preproc my
	tests/scripts/compare_results.pl ref my

# test macro application
test4: clean macro
	rm -f tests
	ln -s tests-pa4 tests
	tests/scripts/run_all_tests.pl macro my
	tests/scripts/compare_results.pl ref my

# test ctrlexpr application
test3: clean ctrlexpr
	rm -f tests
	ln -s tests-pa3 tests
	tests/scripts/run_all_tests.pl ctrlexpr my
	tests/scripts/compare_results.pl ref my

# test posttoken application
test2: clean posttoken
	rm -f tests
	ln -s tests-pa2 tests
	tests/scripts/run_all_tests.pl posttoken my
	tests/scripts/compare_results.pl ref my

# test pptoken application
test1: clean pptoken
	rm -f tests
	ln -s tests-pa1 tests
	tests/scripts/run_all_tests.pl pptoken my
	tests/scripts/compare_results.pl ref my
