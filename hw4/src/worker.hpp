#pragma once
#include <string>
#include <atomic>

struct SearchTask {
    std::string start;
    std::string end;
    std::string target_hash;
};

struct SearchResult {
    bool found;
    std::string key;
};

class Worker {
public:
    explicit Worker(int num_threads);

    SearchResult search(const SearchTask& task, std::atomic<bool>& stop_flag);

    int num_threads() const;

private:
    int num_threads_;
};
