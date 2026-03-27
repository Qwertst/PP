#pragma once

#include "node.hpp"
#include <vector>

class OptimisticIntSet {
    Node* head;
    Node* tail;
    std::vector<Node*> garbage;
    std::mutex garbage_mtx;

    bool validate(Node* prev, Node* curr) {
        Node* node = head;
        while (node->key <= prev->key) {
            if (node == prev) return prev->next == curr;
            node = node->next;
        }
        return false;
    }

public:
    OptimisticIntSet() {
        head = new Node(INT_MIN);
        tail = new Node(INT_MAX);
        head->next = tail;
    }

    ~OptimisticIntSet() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        for (auto* p : garbage) delete p;
    }

    OptimisticIntSet(const OptimisticIntSet&) = delete;
    OptimisticIntSet& operator=(const OptimisticIntSet&) = delete;

    bool add(int key) {
        while (true) {
            Node* prev = head;
            Node* curr = head->next;
            while (curr->key < key) {
                prev = curr;
                curr = curr->next;
            }
            prev->mtx.lock();
            curr->mtx.lock();
            if (validate(prev, curr)) {
                if (curr->key == key) {
                    curr->mtx.unlock();
                    prev->mtx.unlock();
                    return false;
                }
                Node* node = new Node(key);
                node->next = curr;
                prev->next = node;
                curr->mtx.unlock();
                prev->mtx.unlock();
                return true;
            }
            curr->mtx.unlock();
            prev->mtx.unlock();
        }
    }

    bool remove(int key) {
        while (true) {
            Node* prev = head;
            Node* curr = head->next;
            while (curr->key < key) {
                prev = curr;
                curr = curr->next;
            }
            prev->mtx.lock();
            curr->mtx.lock();
            if (validate(prev, curr)) {
                if (curr->key != key) {
                    curr->mtx.unlock();
                    prev->mtx.unlock();
                    return false;
                }
                prev->next = curr->next;
                curr->mtx.unlock();
                prev->mtx.unlock();
                {
                    std::lock_guard<std::mutex> g(garbage_mtx);
                    garbage.push_back(curr);
                }
                return true;
            }
            curr->mtx.unlock();
            prev->mtx.unlock();
        }
    }

    bool contains(int key) {
        while (true) {
            Node* prev = head;
            Node* curr = head->next;
            while (curr->key < key) {
                prev = curr;
                curr = curr->next;
            }
            prev->mtx.lock();
            curr->mtx.lock();
            if (validate(prev, curr)) {
                bool found = (curr->key == key);
                curr->mtx.unlock();
                prev->mtx.unlock();
                return found;
            }
            curr->mtx.unlock();
            prev->mtx.unlock();
        }
    }
};
