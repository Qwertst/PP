#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

struct ServerConfig {
    uint16_t port;
    std::string range_start;
    std::string range_end;
    std::string target_hash;
    int expected_clients;
};

struct ClientState {
    int fd{-1};
    uint64_t hashrate{0};
    std::string recv_buf;
};

class Server {
public:
    explicit Server(ServerConfig config);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();
    void request_stop();

    bool key_found() const;
    std::string found_key() const;

private:
    void handle_client(int client_id);

    ServerConfig config_;
    int listen_fd_{-1};

    std::atomic<bool> stop_{false};
    std::atomic<bool> found_{false};
    std::string found_key_;
    mutable std::mutex found_mutex_;

    std::vector<ClientState> clients_;
    std::vector<std::thread> threads_;
};
