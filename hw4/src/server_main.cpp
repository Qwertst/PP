#include "server.hpp"
#include <iostream>
#include <csignal>

static Server* g_server = nullptr;

static void signal_handler(int) {
    if (g_server) g_server->request_stop();
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <port> <start> <end> <hash> <num_clients>\n";
        return 1;
    }

    int port_int = 0;
    int num_clients = 0;
    try {
        port_int = std::stoi(argv[1]);
        num_clients = std::stoi(argv[5]);
    } catch (...) {
        std::cerr << "Invalid port or num_clients\n";
        return 1;
    }

    if (port_int <= 0 || port_int > 65535) {
        std::cerr << "Port out of range\n";
        return 1;
    }
    if (num_clients <= 0) {
        std::cerr << "num_clients must be > 0\n";
        return 1;
    }

    ServerConfig cfg;
    cfg.port = static_cast<uint16_t>(port_int);
    cfg.range_start = argv[2];
    cfg.range_end = argv[3];
    cfg.target_hash = argv[4];
    cfg.expected_clients = num_clients;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        Server server(cfg);
        g_server = &server;
        server.run();
        g_server = nullptr;
        return server.key_found() ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        g_server = nullptr;
        return 2;
    }
}
