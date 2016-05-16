COMPILE_FLAGS = -std=c++11 -Wall -Werror -Weffc++ -pedantic -ggdb -DEXCLUDE_TESTS
LINK_FLAGS = -lz -lm
CXX = g++

all : fencedfilter

fencedfilter : fencedfilter.o hlindex.o hlnode.o
	$(CXX) -o fencedfilter fencedfilter.o hlindex.o hlnode.o $(LINK_FLAGS)

fencedfilter.o : fencedfilter.cpp hlindex.o
	$(CXX) $(COMPILE_FLAGS) -c -o fencedfilter.o fencedfilter.cpp

hlindex.o : hlindex.hpp hlindex.cpp hlnode.o
	$(CXX) $(COMPILE_FLAGS) -c -o hlindex.o hlindex.cpp

hlnode.o : hlnode.hpp hlnode.cpp
	$(CXX) $(COMPILE_FLAGS) -c -o hlnode.o hlnode.cpp


hl:
	hlfilework/elements_make_hl
	hlfilework/css_make_hl
	hlfilework/css3_make_hl
