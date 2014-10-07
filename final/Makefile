CFLAGS=-g -Wall
CPPFLAGS=-g -Wall -std=gnu++11 -I. 
CC=ccache gcc
CXX=ccache g++

# Now the clang crash problem was gone after the VC++ integration.
# It appears that clang compiled code was a bit slower than the g++ one.
# We need to investigate why the -ftemplate-depth=1024 flag is needed.
# CC=ccache clang
# CXX=ccache clang++
# CPPFLAGS=-g -Wall -std=gnu++11 -I. -fsanitize=address -D__STRICT_ANSI__ -Wno-unused-function -ftemplate-depth=1024 -Wno-parentheses -Qunused-arguments

PPDecoders=preprocessing_token/Utf8Decoder.o preprocessing_token/TrigraphDecoder.o preprocessing_token/UniversalCharNameDecoder.o
PPFSMs=preprocessing_token/PPOpOrPuncFSM.o preprocessing_token/IdentifierFSM.o preprocessing_token/PPNumberFSM.o preprocessing_token/QuotedLiteralFSM.o preprocessing_token/RawStringLiteralFSM.o
PPTokenLib=$(PPDecoders) $(PPFSMs) preprocessing_token/Utf8Utils.o preprocessing_token/Utf8Encoder.o preprocessing_token/PreprocessingToken.o preprocessing_token/PPTokenizerHelper.o preprocessing_token/PPTokenizer.o
PPTokenOBJs=pptoken.o $(PPTokenLib)

Tokenizers=CharLiteralTokenizer.o StringLiteralTokenizer.o FloatLiteralTokenizer.o IntegerLiteralTokenizer.o
TokenLib=Tokenizer.o Utf16Encoder.o Type.o ConstantValue.o Expression.o Member.o Namespace.o DeclSpecifiers.o StorageClass.o $(Tokenizers)
PostTokenOBJs=posttoken.o $(TokenLib) $(PPTokenLib)

CtrlExprOBJs=CtrlExprEval.o ctrlexpr.o $(TokenLib) $(PPTokenLib)

PPDirectiveLib=BuildEnv.o PredefinedMacros.o SourceReader.o CtrlExprEval.o MacroProcessor.o PPDirective.o Preprocessor.o

MacroOBJs=macro.o $(PPDirectiveLib) $(TokenLib) $(PPTokenLib)

PreProcOBJs=preproc.o $(PPDirectiveLib) $(TokenLib) $(PPTokenLib)

ParserLib=Parser.o
RecogOBJs=recog.o $(ParserLib) $(PPDirectiveLib) $(TokenLib) $(PPTokenLib)

NsDeclLib=Driver.o parsers/SemanticParser.o
NsDeclOBJs=nsdecl.o $(NsDeclLib) $(PPDirectiveLib) $(TokenLib) $(PPTokenLib)

NsInitLib=Linker.o
NsInitOBJs=nsinit.o $(NsInitLib) $(NsDeclLib) $(PPDirectiveLib) $(TokenLib) $(PPTokenLib)

Cy86Lib=Cy86Compiler.o Cy86Parser.o Cy86Instruction.o X86Instruction.o
Cy86OBJs=cy86.o $(Cy86Lib) $(PPDirectiveLib) $(TokenLib) $(PPTokenLib)

all: clean pptoken posttoken ctrlexpr macro preproc recog nsdecl nsinit cy86

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
	rm -f *.o parsers/*.o preprocessing_token/*.o pptoken posttoken ctrlexpr macro preproc recog nsdecl nsinit cy86

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
