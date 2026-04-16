#include <atomic>
#include <cstdint>
#include <cstdio>
#include <semaphore>
#include <thread>

#ifndef FENCE
#define FENCE 0
#endif

static std::atomic<int> X{0}, Y{0};
static std::atomic<int> r1{0}, r2{0};

static std::binary_semaphore begin1{0}, begin2{0};
static std::counting_semaphore<2> done{0};
static std::atomic<bool> g_stop{false};

static inline uint32_t xorshift(uint32_t x) {
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

static inline void random_delay(uint32_t &s) {
  while ((s = xorshift(s)) & 7);
}

static void worker1() {
  uint32_t s = 0xdeadbeefu;
  for (;;) {
    begin1.acquire();
    if (g_stop.load(std::memory_order_acquire))
      return;
    random_delay(s);

    X.store(1, std::memory_order_relaxed);
#if FENCE
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
    r1.store(Y.load(std::memory_order_relaxed), std::memory_order_relaxed);

    done.release();
  }
}

static void worker2() {
  uint32_t s = 0x52abcdefu;
  for (;;) {
    begin2.acquire();
    if (g_stop.load(std::memory_order_acquire))
      return;
    random_delay(s);

    Y.store(1, std::memory_order_relaxed);
#if FENCE
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
    r2.store(X.load(std::memory_order_relaxed), std::memory_order_relaxed);

    done.release();
  }
}

int main(int argc, char **argv) {
  long rounds = (argc > 1) ? std::atol(argv[1]) : 1000000;

  std::thread t1(worker1);
  std::thread t2(worker2);

  long detected = 0;
  for (long i = 0; i < rounds; ++i) {
    X.store(0, std::memory_order_relaxed);
    Y.store(0, std::memory_order_relaxed);

    begin1.release();
    begin2.release();
    done.acquire();
    done.acquire();

    if (r1.load(std::memory_order_relaxed) == 0 &&
        r2.load(std::memory_order_relaxed) == 0)
      ++detected;
  }

  g_stop.store(true, std::memory_order_release);
  begin1.release();
  begin2.release();
  t1.join();
  t2.join();

  std::printf("fence = %d rounds = %ld\n", FENCE, rounds);
  std::printf("-- (r1==0 && r2==0) reorderings detected: %ld (1 на %ld раундов) --\n",
              detected, detected ? rounds / detected : 0);
  return 0;
}
