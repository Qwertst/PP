#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <semaphore>
#include <thread>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

#define PLAIN 0
#define RELAXED 1
#define ACQREL 2

#ifndef MODE
#define MODE RELAXED
#endif

#if MODE == PLAIN
using DataT = int;
using FlagT = bool;
static inline void put_data(DataT &d, int v) { d = v; }
static inline void put_flag(FlagT &f) { f = true; }
static inline bool get_flag(const FlagT &f) { return f; }
static inline int get_data(const DataT &d) { return d; }
static inline void reset(DataT &d, FlagT &f) { d = 0; f = false; }
static const char *MODE_NAME = "PLAIN (non-atomic)";

#elif MODE == RELAXED
using DataT = std::atomic<int>;
using FlagT = std::atomic<bool>;
static inline void put_data(DataT &d, int v) { d.store(v, std::memory_order_relaxed); }
static inline void put_flag(FlagT &f) { f.store(true, std::memory_order_relaxed); }
static inline bool get_flag(const FlagT &f) { return f.load(std::memory_order_relaxed); }
static inline int get_data(const DataT &d) { return d.load(std::memory_order_relaxed); }
static inline void reset(DataT &d, FlagT &f) {
  d.store(0, std::memory_order_relaxed);
  f.store(false, std::memory_order_relaxed);
}
static const char *MODE_NAME = "RELAXED atomic";

#else // ACQREL
using DataT = std::atomic<int>;
using FlagT = std::atomic<bool>;
static inline void put_data(DataT &d, int v) { d.store(v, std::memory_order_relaxed); }
static inline void put_flag(FlagT &f) { f.store(true, std::memory_order_release); }
static inline bool get_flag(const FlagT &f) { return f.load(std::memory_order_acquire); }
static inline int get_data(const DataT &d) { return d.load(std::memory_order_relaxed); }
static inline void reset(DataT &d, FlagT &f) {
  d.store(0, std::memory_order_relaxed);
  f.store(false, std::memory_order_relaxed);
}
static const char *MODE_NAME = "ACQUIRE/RELEASE atomic";
#endif

inline constexpr std::size_t CACHELINE = 128;
alignas(CACHELINE) static DataT g_data;
alignas(CACHELINE) static FlagT g_ready;

static std::binary_semaphore beginProd{0};
static std::binary_semaphore beginCons{0};
static std::counting_semaphore<2> done{0};
static std::atomic<bool> g_stop{false};

static std::atomic<long> g_stale{0};
static std::atomic<long> g_missed{0};

static const long SPIN_CAP = 2000000000L;

#ifndef PREP_ITERS
#define PREP_ITERS 2000
#endif
static volatile int g_sink;
static inline void prepare_data() {
  for (int i = 0; i < PREP_ITERS; ++i)
    g_sink = g_sink + 1;
}

static void producer() {
  for (;;) {
    beginProd.acquire();
    if (g_stop.load(std::memory_order_acquire))
      return;
    prepare_data();
    put_data(g_data, 52);
    put_flag(g_ready);
    done.release();
  }
}

static void consumer() {
  for (;;) {
    beginCons.acquire();
    if (g_stop.load(std::memory_order_acquire))
      return;
    long spins = 0;
    bool seen = false;
    while (true) {
      if (get_flag(g_ready)) {
        seen = true;
        break;
      }
      if (++spins >= SPIN_CAP)
        break;
    }
    if (!seen) {
      g_missed.fetch_add(1, std::memory_order_relaxed);
    } else {
      int x = get_data(g_data);
      if (x != 52)
        g_stale.fetch_add(1, std::memory_order_relaxed);
    }
    done.release();
  }
}

#if defined(__linux__)
static void pin_thread(std::thread &t, int cpu) {
  if (cpu < 0)
    return;
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu, &set);
  pthread_setaffinity_np(t.native_handle(), sizeof(set), &set);
}
#else
static void pin_thread(std::thread &, int) {}
#endif

int main(int argc, char **argv) {
  long rounds = (argc > 1) ? std::atol(argv[1]) : 1000000;
  int prod_cpu = (argc > 2) ? std::atoi(argv[2]) : -1;
  int cons_cpu = (argc > 3) ? std::atoi(argv[3]) : -1;

  std::thread prod(producer);
  std::thread cons(consumer);
  pin_thread(prod, prod_cpu);
  pin_thread(cons, cons_cpu);

  for (long i = 0; i < rounds; ++i) {
    reset(g_data, g_ready);
    beginProd.release();
    beginCons.release();
    done.acquire();
    done.acquire();
  }

  g_stop.store(true, std::memory_order_release);
  beginProd.release();
  beginCons.release();
  prod.join();
  cons.join();

  std::printf("mode = %s rounds = %ld\n", MODE_NAME, rounds);
  std::printf("-- stale (ready виден, data == 0) : %ld --\n", g_stale.load());
  std::printf("-- missed (флаг так и не увиден) : %ld --\n", g_missed.load());
  return 0;
}
