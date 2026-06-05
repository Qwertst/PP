CXX ?= g++
CXXSTD ?= -std=c++20
PTHREAD ?= -pthread
WARN ?= -Wall -Wextra
OPT ?= -O2
EXTRA ?=

CXXFLAGS_ALL = $(CXXSTD) $(OPT) $(PTHREAD) $(WARN) $(EXTRA)

.PHONY: all run tsan asan clean

all: hw10

hw10: main.cpp lock_free_set.hpp linearizability.hpp
	$(CXX) $(CXXFLAGS_ALL) -o $@ main.cpp

run: hw10
	./hw10

tsan: main.cpp lock_free_set.hpp linearizability.hpp
	$(CXX) $(CXXSTD) -O1 -g $(PTHREAD) $(WARN) -fsanitize=thread -o hw10_tsan main.cpp

asan: main.cpp lock_free_set.hpp linearizability.hpp
	$(CXX) $(CXXSTD) -O1 -g $(PTHREAD) $(WARN) -fsanitize=address -o hw10_asan main.cpp

clean:
	rm -f hw10 hw10_tsan hw10_asan
