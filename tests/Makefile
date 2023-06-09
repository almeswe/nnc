CC			= gcc
CXX 		= g++
CXXFLAGS 	= -g

GTESTS_SRC  = ./nnc_lex_tests.cpp
GTESTS_OBJ	= ./nnc_lex_tests.o

GTEST 		= ./googletest/

INCLUDE_HDR = $(GTEST)googletest/include
INCLUDE_LIB = $(GTEST)lib

DEPS		= $(INCLUDE_LIB)/libgtest.a 	 \
			  $(INCLUDE_LIB)/libgtest_main.a \

LIBS		= -lpthread -lgtest_main -lgtest 

GTESTS_EXECUTABLE 	= run_gtests

MAP_TESTS_SRC = nnc_tests_main.c 		\
				nnc_test.c 				\
				../src/nnc_buf.c 		\
				../src/nnc_map.c 		\
				../src/nnc_ctx.c		\
				../src/nnc_format.c  	\
				../src/nnc_error.c		\
				../src/nnc_try_catch.c	\
				../src/nnc_arena.c

MAP_TESTS_OBJ = nnc_tests_main.o 	\
				nnc_test.o 			\
				nnc_buf.o 	        \
				nnc_map.o 			\
				nnc_lex.o 			\
				nnc_ctx.o			\
				nnc_format.o 		\
				nnc_error.o			\
				nnc_try_catch.o		\
				nnc_arena.o

MAP_TESTS_EXECUTABLE = run_tests

all: map_tests
	@echo "All tests compiled."

.PHONY: gtests
gtests: gtests_o $(DEPS)
	$(CXX) -L$(INCLUDE_LIB) $(LIBS) $(GTESTS_OBJ) -o $(GTESTS_EXECUTABLE)

.PHONY: gtests_o
gtests_o: $(GTESTS_SRC) $(DEPS)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_HDR) -c $(GTESTS_SRC)

.PHONY: map_tests
map_tests: map_tests_o
	$(CC) $(MAP_TESTS_OBJ) -o $(MAP_TESTS_EXECUTABLE)

.PHONY: map_tests_o
map_tests_o: $(MAP_TESTS_SRC)
	$(CC) $(CXXFLAGS) -c $(MAP_TESTS_SRC)

buf_tests: buf_tests_o
	$(CC) $(CXXFLAGS) -c $(BUF_TESTS_SRC)

.PHONY: clean
clean:
	rm $(GTESTS_OBJ) $(GTESTS_EXECUTABLE)	\
	   $(MAP_TESTS_OBJ) $(MAP_TESTS_EXECUTABLE)

.PHONY: help
help:
	@echo "Available targets:"
	@echo "     ...all"
	@echo "     ...gtests"
	@echo "     ...buf_tests"
	@echo "     ...map_tests"