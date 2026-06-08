CXX ?= g++
CXXSTD ?= -std=c++20
WARN ?= -Wall -Wextra
OPT ?= -O2
PTHREAD ?= -pthread
EXTRA ?=

CXXFLAGS_ALL = $(CXXSTD) $(OPT) $(PTHREAD) $(WARN) $(EXTRA)

REPS ?= 5
THREADS ?= 12
N ?= 10000000

.PHONY: all run scale cutoff clean

all: mergesort

mergesort: mergesort.cpp
	$(CXX) $(CXXFLAGS_ALL) -o $@ $<

run: mergesort
	@for n in 100000 1000000 10000000 50000000 100000000; do \
		./mergesort $$n seq $(REPS); \
		./mergesort $$n par $(REPS) $(THREADS); \
	done

scale: mergesort
	@for t in 1 2 4 8 12; do \
		./mergesort $(N) par $(REPS) $$t; \
	done

cutoff: mergesort
	@for c in 1024 4096 16384 65536 262144 1048576 4194304; do \
		./mergesort $(N) par $(REPS) $(THREADS) $$c; \
	done

clean:
	rm -f mergesort
