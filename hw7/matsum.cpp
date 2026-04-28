#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <vector>

using Matrix = std::vector<std::vector<int>>;

std::int64_t sumRows(const Matrix &m) {
    std::int64_t s = 0;
    const std::size_t n = m.size();
    for (std::size_t i = 0; i < n; ++i) {
        const std::vector<int> &row = m[i];
        for (std::size_t j = 0; j < n; ++j) {
            s += row[j];
        }
    }
    return s;
}

std::int64_t sumCols(const Matrix &m) {
    std::int64_t s = 0;
    const std::size_t n = m.size();
    for (std::size_t j = 0; j < n; ++j) {
        for (std::size_t i = 0; i < n; ++i) {
            s += m[i][j];
        }
    }
    return s;
}

std::int64_t sumRowsFlat(const std::vector<int> &m, std::size_t n) {
    std::int64_t s = 0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            s += m[i * n + j];
        }
    }
    return s;
}

std::int64_t sumColsFlat(const std::vector<int> &m, std::size_t n) {
    std::int64_t s = 0;
    for (std::size_t j = 0; j < n; ++j) {
        for (std::size_t i = 0; i < n; ++i) {
            s += m[i * n + j];
        }
    }
    return s;
}

std::int64_t sumRowsPar(const Matrix &m, unsigned T) {
    const std::size_t n = m.size();
    if (T < 1) {
        T = 1;
    }
    std::vector<std::int64_t> partial(T, 0);
    std::vector<std::thread> th;
    th.reserve(T);
    for (unsigned t = 0; t < T; ++t) {
        const std::size_t lo = n * t / T;
        const std::size_t hi = n * (t + 1) / T;
        th.emplace_back([&, lo, hi, t] {
            std::int64_t s = 0;
            for (std::size_t i = lo; i < hi; ++i) {
                const std::vector<int> &row = m[i];
                for (std::size_t j = 0; j < n; ++j) {
                    s += row[j];
                }
            }
            partial[t] = s;
        });
    }
    for (std::thread &x : th) {
        x.join();
    }
    std::int64_t s = 0;
    for (std::int64_t p : partial) {
        s += p;
    }
    return s;
}

std::int64_t sumColsPar(const Matrix &m, unsigned T) {
    const std::size_t n = m.size();
    if (T < 1) {
        T = 1;
    }
    std::vector<std::vector<std::int64_t>> partial(T);
    std::vector<std::thread> th;
    th.reserve(T);
    for (unsigned t = 0; t < T; ++t) {
        const std::size_t lo = n * t / T;
        const std::size_t hi = n * (t + 1) / T;
        th.emplace_back([&, lo, hi, t] {
            std::vector<std::int64_t> colsum(n, 0);
            for (std::size_t i = lo; i < hi; ++i) {
                const std::vector<int> &row = m[i];
                for (std::size_t j = 0; j < n; ++j) {
                    colsum[j] += row[j];
                }
            }
            partial[t] = std::move(colsum);
        });
    }
    for (std::thread &x : th) {
        x.join();
    }
    std::int64_t s = 0;
    for (unsigned t = 0; t < T; ++t) {
        for (std::size_t j = 0; j < n; ++j) {
            s += partial[t][j];
        }
    }
    return s;
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

    const bool flat = (mode == "flat_rows" || mode == "flat_cols");

    std::mt19937 rng(52);
    std::uniform_int_distribution<int> dist(0, 99);

    Matrix m;
    std::vector<int> mf;
    if (flat) {
        mf.resize(n * n);
        for (auto &x : mf) {
            x = dist(rng);
        }
    } else {
        m.assign(n, std::vector<int>(n));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                m[i][j] = dist(rng);
            }
        }
    }

    std::int64_t check = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) {
        if (mode == "rows") {
            check += sumRows(m);
        } else if (mode == "cols") {
            check += sumCols(m);
        } else if (mode == "flat_rows") {
            check += sumRowsFlat(mf, n);
        } else if (mode == "flat_cols") {
            check += sumColsFlat(mf, n);
        } else if (mode == "rows_par") {
            check += sumRowsPar(m, threads);
        } else if (mode == "cols_par") {
            check += sumColsPar(m, threads);
        } else {
            std::fprintf(stderr, "unknown mode '%s'\n", mode.c_str());
            return 2;
        }
    }
    auto t1 = std::chrono::steady_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double per = ms / reps;
    double bytes = double(n) * n * sizeof(int);
    double gbs = (bytes / (per / 1000.0)) / 1e9;

    const bool par = (mode == "rows_par" || mode == "cols_par");
    std::printf(
        "mode=%-12s N=%zu reps=%d T=%u  check=%lld  %.2f ms total, "
        "%.3f ms/rep, %.2f GB/s\n",
        mode.c_str(), n, reps, par ? threads : 1u, (long long)check, ms, per,
        gbs
    );
    return 0;
}
