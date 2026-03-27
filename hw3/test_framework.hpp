#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>

struct BenchResult {
    std::string name;
    int threads;
    double ops_per_sec;
    long long total_ops;
};

class TestFramework {
public:
    template<typename SetType>
    BenchResult benchmark(const std::string& name, int num_threads, int key_range,
                          double write_ratio, int duration_sec) {
        SetType s;
        for (int i = 0; i < key_range / 2; ++i) {
            s.add(i * 2);
        }

        std::atomic<bool> running{true};
        std::atomic<long long> total_ops{0};

        std::vector<std::thread> threads;
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&s, &running, &total_ops, key_range, write_ratio, t]() {
                std::mt19937 rng(t * 31 + 42);
                std::uniform_int_distribution<int> key_dist(0, key_range - 1);
                std::uniform_real_distribution<double> op_dist(0.0, 1.0);
                long long local_ops = 0;
                while (running.load(std::memory_order_relaxed)) {
                    int key = key_dist(rng);
                    double op = op_dist(rng);
                    if (op < write_ratio / 2.0) {
                        s.add(key);
                    } else if (op < write_ratio) {
                        s.remove(key);
                    } else {
                        s.contains(key);
                    }
                    ++local_ops;
                }
                total_ops.fetch_add(local_ops, std::memory_order_relaxed);
            });
        }

        std::this_thread::sleep_for(std::chrono::seconds(duration_sec));
        running.store(false, std::memory_order_relaxed);
        for (auto& th : threads) th.join();

        long long ops = total_ops.load();
        double ops_sec = static_cast<double>(ops) / duration_sec;
        return {name, num_threads, ops_sec, ops};
    }

    static void print_bench_table(const std::vector<BenchResult>& results) {
        std::cout << std::left << std::setw(22) << "Strategy"
                  << std::right << std::setw(8) << "Threads"
                  << std::setw(18) << "Ops/sec"
                  << std::setw(16) << "Total ops" << "\n";
        std::cout << std::string(64, '-') << "\n";
        for (auto& r : results) {
            std::cout << std::left << std::setw(22) << r.name
                      << std::right << std::setw(8) << r.threads
                      << std::setw(18) << std::fixed << std::setprecision(0) << r.ops_per_sec
                      << std::setw(16) << r.total_ops << "\n";
        }
        std::cout << "\n";
    }
};
