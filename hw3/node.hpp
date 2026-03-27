#pragma once

#include <climits>
#include <mutex>

struct Node {
    int key;
    Node* next;
    std::mutex mtx;
    bool marked;

    Node(int k) : key(k), next(nullptr), marked(false) {}
};
