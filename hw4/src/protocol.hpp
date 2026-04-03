#pragma once
#include <string>
#include <cstdint>

enum class ServerCommand {
    TASK,
    WAIT,
    DONE,
    UNKNOWN
};

struct TaskMessage {
    std::string start;
    std::string end;
    std::string target_hash;
};

struct ServerMessage {
    ServerCommand command;
    TaskMessage task;
    uint32_t wait_seconds{0};
};

class Protocol {
public:
    static std::string encode_hello(uint64_t hashrate);
    static std::string encode_found(const std::string& key);
    static std::string encode_not_found();
    static ServerMessage decode(const std::string& line);
};
