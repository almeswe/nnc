CXX 		= g++
CXXFLAGS 	= -g

SRC  		= ./*.cpp
OBJ			= ./*.o

GTEST 		= ./googletest/

INCLUDE_HDR = $(GTEST)googletest/include
INCLUDE_LIB = $(GTEST)lib

DEPS		= $(INCLUDE_LIB)/libgtest.a 	 \
			  $(INCLUDE_LIB)/libgtest_main.a \

LIBS		= -lpthread -lgtest_main -lgtest 

EXECUTABLE 	= run_tests

.PHONY: run
.PHONY: tests
.PHONY: clean

run: tests $(DEPS)
	$(CXX) -L$(INCLUDE_LIB) $(LIBS) $(OBJ) -o $(EXECUTABLE)

tests: $(SRC) $(DEPS)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_HDR) -c $(SRC)

clean:
	rm $(OBJ) $(EXECUTABLE)