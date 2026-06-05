#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <vector>
#include "linearizability.hpp"
#include "lock_free_set.hpp"

static int g_failures = 0;

static long envInt(const char *name, long def) {
    const char *v = std::getenv(name);
    return v ? std::atol(v) : def;
}

static void linearizabilityTest() {
    std::cout << "--- Linearizability check (stress) ---\n";

    std::mt19937 rng(52);
    const int kScenarios = (int)envInt("HW10_SCEN", 2000);
    const int kRunsPerScenario = (int)envInt("HW10_RUNS", 200);
    const int kDomain = 3;
    const std::array<Op::Type, 4> kTypes = {
        Op::ADD, Op::REMOVE, Op::CONTAINS, Op::IS_EMPTY
    };

    long long total_runs = 0, violations = 0;
    long long covered_total = 0, valid_total = 0;

    std::uniform_int_distribution<int> nOps(3, 6);
    std::uniform_int_distribution<int> typeDist(0, (int)kTypes.size() - 1);
    std::uniform_int_distribution<int> opndDist(0, kDomain - 1);
    std::uniform_int_distribution<int> coin(0, 1);

    for (int sc = 0; sc < kScenarios; ++sc) {
        std::vector<int> init;
        for (int v = 0; v < kDomain; ++v) {
            if (coin(rng)) {
                init.push_back(v);
            }
        }

        int n = nOps(rng);
        std::vector<Op> ops(n);
        for (int i = 0; i < n; ++i) {
            ops[i] = {kTypes[typeDist(rng)], opndDist(rng)};
        }

        std::set<Trace> valid = allValidTraces(init, ops);
        valid_total += (long long)valid.size();

        std::set<Trace> seen;
        for (int r = 0; r < kRunsPerScenario; ++r) {
            Trace obs = runConcurrent(init, ops);
            ++total_runs;
            seen.insert(obs);
            if (valid.find(obs) == valid.end()) {
                ++violations;
                if (violations <= 5) {
                    std::cout << " VIOLATION: init={";
                    for (size_t i = 0; i < init.size(); ++i) {
                        std::cout << (i ? "," : "") << init[i];
                    }
                    std::cout << "}\n  observed: " << traceStr(ops, obs)
                              << "\n valid traces: " << valid.size() << "\n";
                }
            }
        }
        covered_total += (long long)seen.size();
    }

    std::cout << " scenarios: " << kScenarios << "\n";
    std::cout << " concurrent runs: " << total_runs << "\n";
    std::cout << " linearizability violations:" << violations << "\n";
    std::cout << " avg valid traces/scenario: "
              << (double)valid_total / kScenarios << "\n";
    std::cout << "  avg observed traces/scenario: "
              << (double)covered_total / kScenarios << "  (>1 => overlap)\n";
    std::cout << (violations ? "  -> LINEARIZABILITY VIOLATED\n"
                             : "  -> OK: all runs linearizable\n")
              << "\n";
    if (violations) {
        ++g_failures;
    }
}

static void demoTraces() {
    std::cout << "--- Example: all valid traces of one set ---\n";
    std::vector<int> init = {1};
    std::vector<Op> ops = {{Op::ADD, 1}, {Op::REMOVE, 1}, {Op::CONTAINS, 1}};
    std::cout << " initial state: {1}\n  operations: ";
    for (size_t i = 0; i < ops.size(); ++i) {
        std::cout << (i ? ", " : "") << ops[i].str();
    }
    std::cout << "\n valid traces:\n";
    for (const auto &t : allValidTraces(init, ops)) {
        std::cout << "    " << traceStr(ops, t) << "\n";
    }
    std::cout << "\n";
}

int main() {
    demoTraces();
    linearizabilityTest();

    std::cout
        << (g_failures ? "RESULT: failures\n" : "RESULT: all checks passed\n");
    return g_failures ? 1 : 0;
}
