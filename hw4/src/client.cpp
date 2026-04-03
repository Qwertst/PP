#include "client.hpp"
#include "protocol.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() {
    close();
}

void TcpClient::connect(const std::string& host, uint16_t port) {
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (rc != 0) {
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(rc));
    }

    for (auto* p = res; p != nullptr; p = p->ai_next) {
        fd_ = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd_ < 0) continue;
        if (::connect(fd_, p->ai_addr, p->ai_addrlen) == 0) break;
        ::close(fd_);
        fd_ = -1;
    }
    freeaddrinfo(res);

    if (fd_ < 0) {
        throw std::runtime_error("Failed to connect to " + host + ":" + std::to_string(port));
    }
}

void TcpClient::send_line(const std::string& line) {
    size_t total = 0;
    while (total < line.size()) {
        ssize_t n = ::send(fd_, line.data() + total, line.size() - total, 0);
        if (n <= 0) throw std::runtime_error("send failed");
        total += static_cast<size_t>(n);
    }
}

std::string TcpClient::recv_line() {
    while (true) {
        auto pos = recv_buf_.find('\n');
        if (pos != std::string::npos) {
            std::string line = recv_buf_.substr(0, pos);
            recv_buf_.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return line;
        }
        char buf[4096];
        ssize_t n = ::recv(fd_, buf, sizeof(buf), 0);
        if (n <= 0) throw std::runtime_error("Connection closed by server");
        recv_buf_.append(buf, static_cast<size_t>(n));
    }
}

void TcpClient::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool TcpClient::is_connected() const {
    return fd_ >= 0;
}

Client::Client(const std::string& host, uint16_t port, uint64_t hashrate, int num_threads)
    : host_(host), port_(port), hashrate_(hashrate), worker_(num_threads) {}

void Client::request_stop() {
    stop_ = true;
}

void Client::run() {
    tcp_.connect(host_, port_);
    tcp_.send_line(Protocol::encode_hello(hashrate_));

    while (!stop_) {
        std::string line;
        try {
            line = tcp_.recv_line();
        } catch (const std::exception& e) {
            std::cerr << "Network error: " << e.what() << "\n";
            break;
        }

        if (line.empty()) continue;

        ServerMessage msg = Protocol::decode(line);

        if (msg.command == ServerCommand::DONE) {
            std::cout << "Server sent DONE. Exiting.\n";
            break;
        } else if (msg.command == ServerCommand::WAIT) {
            std::cout << "Waiting " << msg.wait_seconds << " seconds...\n";
            for (uint32_t i = 0; i < msg.wait_seconds && !stop_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!stop_) {
                tcp_.send_line(Protocol::encode_hello(hashrate_));
            }
        } else if (msg.command == ServerCommand::TASK) {
            std::cout << "Task received: [" << msg.task.start << ", " << msg.task.end
                      << "] hash=" << msg.task.target_hash << "\n";

            SearchTask task{msg.task.start, msg.task.end, msg.task.target_hash};
            std::atomic<bool> search_stop{false};

            auto stop_watcher = std::thread([&]() {
                while (!stop_ && !search_stop.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                search_stop.store(true);
            });

            SearchResult result = worker_.search(task, search_stop);
            search_stop.store(true, std::memory_order_relaxed);
            stop_watcher.join();

            if (stop_) break;

            if (result.found) {
                std::cout << "Found key: " << result.key << "\n";
                tcp_.send_line(Protocol::encode_found(result.key));
            } else {
                std::cout << "Key not found in range.\n";
                tcp_.send_line(Protocol::encode_not_found());
            }
        } else {
            std::cerr << "Unknown server message: " << line << "\n";
        }
    }

    tcp_.close();
}
