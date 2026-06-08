#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <thread>
#include <vector>

static void
merge(int *a, int *tmp, std::size_t lo, std::size_t mid, std::size_t hi) {
    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (a[i] <= a[j]) {
            tmp[k++] = a[i++];
        } else {
            tmp[k++] = a[j++];
        }
    }
    while (i < mid) {
        tmp[k++] = a[i++];
    }
    while (j < hi) {
        tmp[k++] = a[j++];
    }
    for (std::size_t t = lo; t < hi; ++t) {
        a[t] = tmp[t];
    }
}

static void mergeSortSeq(int *a, int *tmp, std::size_t lo, std::size_t hi) {
    if (hi - lo <= 1) {
        return;
    }
    std::size_t mid = lo + (hi - lo) / 2;
    mergeSortSeq(a, tmp, lo, mid);
    mergeSortSeq(a, tmp, mid, hi);
    merge(a, tmp, lo, mid, hi);
}

static void mergeSortPar(
    int *a, int *tmp, std::size_t lo, std::size_t hi, int depth,
    std::size_t cutoff
) {
    if (depth <= 0 || hi - lo <= cutoff) {
        mergeSortSeq(a, tmp, lo, hi);
        return;
    }
    std::size_t mid = lo + (hi - lo) / 2;
    std::thread left([&] { mergeSortPar(a, tmp, lo, mid, depth - 1, cutoff); });
    mergeSortPar(a, tmp, mid, hi, depth - 1, cutoff);
    left.join();
    merge(a, tmp, lo, mid, hi);
}

int main(int argc, char **argv) {
    const std::size_t n = std::strtoul(argv[1], nullptr, 10);
    const std::string mode = argv[2];
    const int reps = argc > 3 ? std::atoi(argv[3]) : 5;
    unsigned threads = argc > 4 ? (unsigned)std::atoi(argv[4])
                                : std::thread::hardware_concurrency();
    if (threads < 1) {
        threads = 1;
    }
    const std::size_t cutoff =
        argc > 5 ? std::strtoul(argv[5], nullptr, 10) : (1u << 15);

    int depth = 0;
    for (unsigned t = 1; t < threads; t <<= 1) {
        ++depth;
    }

    std::mt19937 rng(52);
    std::uniform_int_distribution<int> dist(0, 1000000000);
    std::vector<int> orig(n);
    for (int &x : orig) {
        x = dist(rng);
    }

    std::vector<int> work(n);
    std::vector<int> tmp(n);
    bool ok = true;
    double ms = 0.0;
    for (int r = 0; r < reps; ++r) {
        work = orig;
        auto t0 = std::chrono::steady_clock::now();
        if (mode == "seq") {
            mergeSortSeq(work.data(), tmp.data(), 0, n);
        } else if (mode == "par") {
            mergeSortPar(work.data(), tmp.data(), 0, n, depth, cutoff);
        } else {
            std::fprintf(stderr, "unknown mode '%s'\n", mode.c_str());
            return 2;
        }
        auto t1 = std::chrono::steady_clock::now();
        ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (!std::is_sorted(work.begin(), work.end())) {
            ok = false;
        }
    }

    double per = ms / reps;
    const bool par = (mode == "par");
    std::printf(
        "mode=%-4s N=%zu reps=%d T=%u cutoff=%zu  sorted=%s  %.2f ms/rep\n",
        mode.c_str(), n, reps, par ? threads : 1u, par ? cutoff : n,
        ok ? "yes" : "NO", per
    );
    return ok ? 0 : 1;
}
