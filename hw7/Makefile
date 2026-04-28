CXX ?= g++
CXXSTD ?= -std=c++20
WARN ?= -Wall -Wextra
OPT ?= -O2
PTHREAD ?= -pthread
EXTRA ?=

CXXFLAGS_ALL = $(CXXSTD) $(OPT) $(PTHREAD) $(WARN) $(EXTRA)

N ?= 4096
REPS ?= 5
THREADS ?= 12
PERF_EV ?= cache-misses,cache-references,cycles,instructions
PERF_EV_L1 ?= L1-dcache-loads,L1-dcache-load-misses,cycles,instructions
PIN ?= taskset -c 1

PIN_PAR ?= taskset -c 0-11

.PHONY: all clean run run-par scale perf perf-all perf-l1 perf-par

all: matsum

matsum: matsum.cpp
	$(CXX) $(CXXFLAGS_ALL) -o $@ $<

run: matsum
	@for mode in rows cols flat_rows flat_cols; do \
		$(PIN) ./matsum $(N) $$mode $(REPS); \
	done

run-par: matsum
	@for mode in rows cols rows_par cols_par; do \
		$(PIN_PAR) ./matsum $(N) $$mode $(REPS) $(THREADS); \
	done

scale: matsum
	@for t in 1 2 4 6 8 12; do \
		$(PIN_PAR) ./matsum $(N) rows_par $(REPS) $$t; \
	done

MODE ?= cols
perf: matsum
	sudo perf stat -e $(PERF_EV) $(PIN) ./matsum $(N) $(MODE) $(REPS)

perf-par: matsum
	sudo perf stat -e $(PERF_EV) $(PIN_PAR) ./matsum $(N) $(MODE) $(REPS) $(THREADS)

perf-all: matsum
	@for mode in rows cols flat_rows flat_cols; do \
		echo "==== $$mode ===="; \
		sudo perf stat -e $(PERF_EV) $(PIN) ./matsum $(N) $$mode $(REPS); \
	done

perf-l1: matsum
	@for mode in rows cols; do \
		echo "==== $$mode ===="; \
		sudo perf stat -e $(PERF_EV_L1) $(PIN) ./matsum $(N) $$mode $(REPS); \
	done

clean:
	rm -f matsum
