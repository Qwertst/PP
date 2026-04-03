#include "server.hpp"
#include "key.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

static std::string do_recv_line(int fd, std::string& buf) {
    while (true) {
        auto pos = buf.find('\n');
        if (pos != std::string::npos) {
            std::string line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return line;
        }
        char tmp[4096];
        ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
        if (n <= 0) throw std::runtime_error("client disconnected");
        buf.append(tmp, static_cast<size_t>(n));
    }
}

static void do_send_line(int fd, const std::string& line) {
    size_t total = 0;
    while (total < line.size()) {
        ssize_t n = ::send(fd, line.data() + total, line.size() - total, 0);
        if (n <= 0) throw std::runtime_error("send failed");
        total += static_cast<size_t>(n);
    }
}

Server::Server(ServerConfig config) : config_(std::move(config)) {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) throw std::runtime_error("socket() failed");

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config_.port);

    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(listen_fd_);
        throw std::runtime_error("bind() failed on port " + std::to_string(config_.port));
    }

    if (::listen(listen_fd_, config_.expected_clients + 1) < 0) {
        ::close(listen_fd_);
        throw std::runtime_error("listen() failed");
    }
}

Server::~Server() {
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
}

void Server::request_stop() {
    stop_.store(true);
}

bool Server::key_found() const {
    return found_.load();
}

std::string Server::found_key() const {
    std::lock_guard<std::mutex> lock(found_mutex_);
    return found_key_;
}

void Server::handle_client(int client_id) {
    ClientState& cs = clients_[client_id];
    int fd = cs.fd;

    try {
        std::string hello = do_recv_line(fd, cs.recv_buf);
        if (hello.size() < 7 || hello.substr(0, 6) != "HELLO ") {
            throw std::runtime_error("Expected HELLO, got: " + hello);
        }
        cs.hashrate = std::stoull(hello.substr(6));
        std::cout << "[server] client " << client_id << " hashrate=" << cs.hashrate << "\n";

        if (found_.load()) {
            do_send_line(fd, "DONE\n");
            ::close(fd);
            return;
        }

        Key full_start(config_.range_start);
        Key full_end(config_.range_end);

        int n = config_.expected_clients;
        uint64_t chunk = 1000000ULL;
        Key seg_start = full_start.advanced(static_cast<uint64_t>(client_id) * chunk);
        Key seg_end = (client_id == n - 1)
            ? full_end
            : full_start.advanced(static_cast<uint64_t>(client_id + 1) * chunk - 1);

        if (!(seg_start <= full_end)) seg_start = full_end;
        if (!(seg_end <= full_end)) seg_end = full_end;

        std::string task_msg = "TASK " + seg_start.str() + " " + seg_end.str()
                             + " " + config_.target_hash + "\n";
        do_send_line(fd, task_msg);
        std::cout << "[server] client " << client_id << " task ["
                  << seg_start.str() << ", " << seg_end.str() << "]\n";

        std::string response = do_recv_line(fd, cs.recv_buf);
        std::cout << "[server] client " << client_id << " response: " << response << "\n";

        if (response.size() > 6 && response.substr(0, 6) == "FOUND ") {
            std::string key = response.substr(6);
            std::lock_guard<std::mutex> lock(found_mutex_);
            if (!found_.load()) {
                found_.store(true);
                found_key_ = key;
                std::cout << "[server] KEY FOUND: " << key << "\n";
            }
        }

        do_send_line(fd, "DONE\n");

    } catch (const std::exception& e) {
        std::cerr << "[server] client " << client_id << " error: " << e.what() << "\n";
    }

    ::close(fd);
}

void Server::run() {
    std::cout << "[server] listening on port " << config_.port
              << " expecting " << config_.expected_clients << " clients\n";
    std::cout.flush();

    clients_.resize(config_.expected_clients);

    for (int i = 0; i < config_.expected_clients && !stop_.load(); ++i) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int fd = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (fd < 0) {
            if (stop_.load()) break;
            std::cerr << "[server] accept() failed\n";
            continue;
        }
        clients_[i].fd = fd;
        std::cout << "[server] client " << i << " connected\n";
        std::cout.flush();
        threads_.emplace_back(&Server::handle_client, this, i);
    }

    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }

    std::cout << "[server] all clients finished. found=" << found_.load() << "\n";
    if (found_.load()) {
        std::cout << "RESULT " << found_key() << "\n";
    } else {
        std::cout << "RESULT NOT_FOUND\n";
    }
    std::cout.flush();
}
