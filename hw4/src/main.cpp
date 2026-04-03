#include "client.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <csignal>
#include <stdexcept>
#include <thread>

static Client* g_client = nullptr;

static void signal_handler(int) {
    if (g_client) {
        g_client->request_stop();
    }
}

static uint64_t read_hashrate(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open hashrate file: " + path);
    }
    uint64_t rate = 0;
    f >> rate;
    if (!f || rate == 0) {
        throw std::runtime_error("Invalid hashrate value in file: " + path);
    }
    return rate;
}

static int num_threads() {
    int n = static_cast<int>(std::thread::hardware_concurrency());
    return n > 0 ? n : 1;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <path_to_hashrate_file>\n";
        return 1;
    }

    std::string host = argv[1];
    int port_int = 0;
    try {
        port_int = std::stoi(argv[2]);
    } catch (...) {
        std::cerr << "Invalid port: " << argv[2] << "\n";
        return 1;
    }
    if (port_int <= 0 || port_int > 65535) {
        std::cerr << "Port out of range: " << port_int << "\n";
        return 1;
    }

    uint64_t hashrate = 0;
    try {
        hashrate = read_hashrate(argv[3]);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGHUP, signal_handler);

    Client client(host, static_cast<uint16_t>(port_int), hashrate, num_threads());
    g_client = &client;

    try {
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        g_client = nullptr;
        return 1;
    }

    g_client = nullptr;
    return 0;
}
