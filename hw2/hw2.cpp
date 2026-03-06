#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>

class DanceFloor {
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<int> leaders_queue_;
    std::queue<int> followers_queue_;

public:
    void leader_arrives(int id) {
        std::unique_lock<std::mutex> lock(mtx_);
        std::cout << "Leader " << id << " arrives\n";

        if (!followers_queue_.empty()) {
            int follower_id = followers_queue_.front();
            followers_queue_.pop();
            std::cout << ">>> Leader " << id << " and Follower " << follower_id << " are DANCING TOGETHER! <<<\n\n";
        } else {
            leaders_queue_.push(id);
            std::cout << "Leader " << id << " waits in queue\n";
        }
        cv_.notify_all();
    }

    void follower_arrives(int id) {
        std::unique_lock<std::mutex> lock(mtx_);
        std::cout << "Follower " << id << " arrives\n";

        if (!leaders_queue_.empty()) {
            int leader_id = leaders_queue_.front();
            leaders_queue_.pop();
            std::cout << ">>> Leader " << leader_id << " and Follower " << id << " are DANCING TOGETHER! <<<\n\n";
        } else {
            followers_queue_.push(id);
            std::cout << "Follower " << id << " waits in queue\n";
        }
        cv_.notify_all();
    }
};

void test() {
    DanceFloor floor;
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&floor, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
            floor.leader_arrives(i);
        });
    }

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&floor, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
            floor.follower_arrives(i);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "All dancers have finished\n";
}

int main() {
    srand(time(nullptr));
    test();
    return 0;
}
