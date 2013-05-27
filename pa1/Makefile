all: pptoken

# build pptoken application
pptoken: pptoken.cpp IPPTokenStream.h DebugPPTokenStream.h Utf8Decoder.h Utf8Decoder.cpp Exceptions.h
	g++ -g -std=gnu++11 -Wall -o pptoken pptoken.cpp Utf8Decoder.cpp

format: format.h format-test.cpp Exceptions.h
	g++ -g -std=gnu++11 -Wall -o format format-test.cpp

# test pptoken application
test: all
	scripts/run_all_tests.pl pptoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl pptoken-ref ref

