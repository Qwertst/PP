#pragma once

#include "node.hpp"
#include <vector>

class LazyIntSet {
    Node* head;
    Node* tail;
    std::vector<Node*> garbage;
    std::mutex garbage_mtx;

    bool validate(Node* prev, Node* curr) {
        return !prev->marked && !curr->marked && prev->next == curr;
    }

public:
    LazyIntSet() {
        head = new Node(INT_MIN);
        tail = new Node(INT_MAX);
        head->next = tail;
    }

    ~LazyIntSet() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        for (auto* p : garbage) delete p;
    }

    LazyIntSet(const LazyIntSet&) = delete;
    LazyIntSet& operator=(const LazyIntSet&) = delete;

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
                curr->marked = true;
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
        Node* curr = head->next;
        while (curr->key < key) {
            curr = curr->next;
        }
        return curr->key == key && !curr->marked;
    }
};
