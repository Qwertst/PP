#include "protocol.hpp"
#include <sstream>

std::string Protocol::encode_hello(uint64_t hashrate) {
    return "HELLO " + std::to_string(hashrate) + "\n";
}

std::string Protocol::encode_found(const std::string& key) {
    return "FOUND " + key + "\n";
}

std::string Protocol::encode_not_found() {
    return "NOT_FOUND\n";
}

ServerMessage Protocol::decode(const std::string& line) {
    ServerMessage msg;
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "TASK") {
        msg.command = ServerCommand::TASK;
        iss >> msg.task.start >> msg.task.end >> msg.task.target_hash;
        if (msg.task.start.empty() || msg.task.end.empty() || msg.task.target_hash.empty()) {
            msg.command = ServerCommand::UNKNOWN;
        }
    } else if (cmd == "WAIT") {
        msg.command = ServerCommand::WAIT;
        iss >> msg.wait_seconds;
        if (msg.wait_seconds == 0) msg.wait_seconds = 1;
    } else if (cmd == "DONE") {
        msg.command = ServerCommand::DONE;
    } else {
        msg.command = ServerCommand::UNKNOWN;
    }

    return msg;
}
