CXX ?= g++
CXXSTD ?= -std=c++20
PTHREAD ?= -pthread
WARN ?= -Wall -Wextra
EXTRA ?=

CXXFLAGS_ALL = $(CXXSTD) $(PTHREAD) $(WARN) $(EXTRA)

PC_MODES = plain_O0 plain_O2 relaxed acqrel

.PHONY: all clean run run-pc run-sl run-arm asm

all: $(PC_MODES) storeload storeload_fence

plain_O0: reorder.cpp
	$(CXX) -O0 $(CXXFLAGS_ALL) -DMODE=PLAIN -o $@ $<

plain_O2: reorder.cpp
	$(CXX) -O2 $(CXXFLAGS_ALL) -DMODE=PLAIN -o $@ $<

relaxed: reorder.cpp
	$(CXX) -O2 $(CXXFLAGS_ALL) -DMODE=RELAXED -o $@ $<

acqrel: reorder.cpp
	$(CXX) -O2 $(CXXFLAGS_ALL) -DMODE=ACQREL -o $@ $<

storeload: storeload.cpp
	$(CXX) -O2 $(CXXFLAGS_ALL) -DFENCE=0 -o $@ $<

storeload_fence: storeload.cpp
	$(CXX) -O2 $(CXXFLAGS_ALL) -DFENCE=1 -o $@ $<

run: run-pc run-sl

run-pc: relaxed acqrel plain_O0
	@echo "== producer/consumer =="
	./relaxed 1000000
	./acqrel 1000000
	@echo "-- PLAIN -O0 --"
	./plain_O0 1000000

run-sl: storeload storeload_fence
	@echo "== store->load =="
	./storeload 1000000
	./storeload_fence 1000000

CPU0 ?= 0
CPU1 ?= 1
ROUNDS ?= 50000000
run-arm:
	rm -f relaxed acqrel
	$(MAKE) relaxed acqrel EXTRA="-DPREP_ITERS=0"
	@echo "== producer/consumer на ARM (cpu $(CPU0) / $(CPU1)) =="
	./relaxed $(ROUNDS) $(CPU0) $(CPU1)
	./acqrel $(ROUNDS) $(CPU0) $(CPU1)

clean:
	rm -f $(PC_MODES) storeload storeload_fence reorder_plain_O2.s
