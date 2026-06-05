#pragma once

#include <algorithm>
#include <atomic>
#include <numeric>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "lock_free_set.hpp"

struct Op {
    enum Type { ADD, REMOVE, CONTAINS, IS_EMPTY } type;

    int operand;

    std::string str() const {
        switch (type) {
            case ADD:
                return "add(" + std::to_string(operand) + ")";
            case REMOVE:
                return "remove(" + std::to_string(operand) + ")";
            case CONTAINS:
                return "contains(" + std::to_string(operand) + ")";
            case IS_EMPTY:
                return "isEmpty()";
        }
        return "?";
    }
};

using Trace = std::vector<char>;

inline bool applyRef(std::set<int> &m, const Op &op) {
    switch (op.type) {
        case Op::ADD:
            return m.insert(op.operand).second;
        case Op::REMOVE:
            return m.erase(op.operand) > 0;
        case Op::CONTAINS:
            return m.count(op.operand) > 0;
        case Op::IS_EMPTY:
            return m.empty();
    }
    return false;
}

inline bool applyConc(LockFreeSet<int> &s, const Op &op) {
    switch (op.type) {
        case Op::ADD:
            return s.add(op.operand);
        case Op::REMOVE:
            return s.remove(op.operand);
        case Op::CONTAINS:
            return s.contains(op.operand);
        case Op::IS_EMPTY:
            return s.isEmpty();
    }
    return false;
}

inline std::set<Trace>
allValidTraces(const std::vector<int> &init, const std::vector<Op> &ops) {
    const int n = static_cast<int>(ops.size());
    std::set<Trace> valid;
    std::vector<int> perm(n);
    std::iota(perm.begin(), perm.end(), 0);
    do {
        std::set<int> m(init.begin(), init.end());
        Trace res(n);
        for (int k : perm) {
            res[k] = applyRef(m, ops[k]) ? 1 : 0;
        }
        valid.insert(res);
    } while (std::next_permutation(perm.begin(), perm.end()));
    return valid;
}

inline Trace
runConcurrent(const std::vector<int> &init, const std::vector<Op> &ops) {
    const int n = static_cast<int>(ops.size());
    LockFreeSet<int> s;
    for (int x : init) {
        s.add(x);
    }

    Trace res(n);
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    threads.reserve(n);
    for (int i = 0; i < n; ++i) {
        threads.emplace_back([&, i]() {
            ready.fetch_add(1, std::memory_order_acq_rel);
            while (!go.load(std::memory_order_acquire)) {
            }
            res[i] = applyConc(s, ops[i]) ? 1 : 0;
        });
    }
    while (ready.load(std::memory_order_acquire) < n) {
    }
    go.store(true, std::memory_order_release);
    for (auto &t : threads) {
        t.join();
    }
    return res;
}

inline std::string traceStr(const std::vector<Op> &ops, const Trace &t) {
    std::string s = "[";
    for (size_t i = 0; i < ops.size(); ++i) {
        if (i) {
            s += ", ";
        }
        s += "(" + ops[i].str() + ":" + (t[i] ? "true" : "false") + ")";
    }
    return s + "]";
}
