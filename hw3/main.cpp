#include <iostream>
#include <vector>

#include "coarse_grained_int_set.hpp"
#include "fine_grained_int_set.hpp"
#include "optimistic_int_set.hpp"
#include "lazy_int_set.hpp"
#include "test_framework.hpp"

int main() {
    TestFramework fw;

    std::cout << "=== Benchmark: 20% write / 80% read ===\n\n";

    const int key_range = 1024;
    const double write_ratio = 0.2;
    const int bench_duration = 3;
    std::vector<int> thread_counts = {1, 2, 4, 8};

    for (int nt : thread_counts) {
        std::cout << "Threads: " << nt << "\n";
        std::vector<BenchResult> results;

        results.push_back(fw.benchmark<CoarseGrainedIntSet>(
            "Coarse-grained", nt, key_range, write_ratio, bench_duration));
        results.push_back(fw.benchmark<FineGrainedIntSet>(
            "Fine-grained", nt, key_range, write_ratio, bench_duration));
        results.push_back(fw.benchmark<OptimisticIntSet>(
            "Optimistic", nt, key_range, write_ratio, bench_duration));
        results.push_back(fw.benchmark<LazyIntSet>(
            "Lazy", nt, key_range, write_ratio, bench_duration));

        TestFramework::print_bench_table(results);
    }

    return 0;
}
