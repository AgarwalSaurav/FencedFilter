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


# Build highlighting files from internet sources:
hl:
	cd hlfilework      && \
	./elements_make_hl && \
	./css_make_hl      && \
	./css3_make_hl

# Delete all generated content from the directory
clean:
	rm -f fencedfilter # executable
	rm -f *.o          # object files
	rm -f hlindex      # unit test file
	rm -f hlnode       # unit test file
	rm -f css.hl       # css highlighting file from `make hl` target
	rm -f css3.hl      # css highlighting file from `make hl` target
	rm -f elements.hl  # elements highlighting file from `make hl` target
	rm -f -r html      # Output of Doxygen
