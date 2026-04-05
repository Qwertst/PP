#include <iostream>
#include <thread>
#include <vector>
#include <semaphore>
#include <atomic>
#include <algorithm>

std::counting_semaphore<> h_sem{0};
std::counting_semaphore<> o_sem{0};
std::counting_semaphore<> done{0};

std::atomic<bool> finished{false}; // чтобы не выводить лишнее в случае N потоков

void hydrogen() {
    h_sem.acquire();
    if (finished.load()) return;
    std::cout << 'H';
    done.release();
}

void oxygen() {
    o_sem.acquire();
    if (finished.load()) return;
    std::cout << 'O';
    done.release();
}

int main() {
    const bool is_hydrogen[6] = {true, true, false, true, false, true}; // конфигурация из условия

    int n_h = 0, n_o = 0;
    for (bool h : is_hydrogen) (h ? n_h : n_o)++;

    std::vector<std::thread> threads;
    for (bool h : is_hydrogen)
        threads.emplace_back(h ? hydrogen : oxygen);

    const int molecules = std::min(n_h / 2, n_o);

    for (int i = 0; i < molecules; ++i) {
        h_sem.release(2);
        o_sem.release(1);
        done.acquire();
        done.acquire();
        done.acquire();
    }

    finished.store(true);
    h_sem.release(n_h);
    o_sem.release(n_o);

    for (auto& t : threads) t.join();

    std::cout << std::endl;
    return 0;
}
