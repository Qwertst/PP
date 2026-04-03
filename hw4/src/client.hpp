#pragma once
#include "worker.hpp"
#include <string>

class TcpClient {
public:
    TcpClient();
    ~TcpClient();

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    void connect(const std::string& host, uint16_t port);
    void send_line(const std::string& line);
    std::string recv_line();
    void close();
    bool is_connected() const;

private:
    int fd_{-1};
    std::string recv_buf_;
};

class Client {
public:
    Client(const std::string& host, uint16_t port, uint64_t hashrate, int num_threads);

    void run();
    void request_stop();

private:
    std::string host_;
    uint16_t port_;
    uint64_t hashrate_;
    Worker worker_;
    TcpClient tcp_;
    volatile bool stop_{false};
};
