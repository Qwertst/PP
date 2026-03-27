#pragma once

#include "node.hpp"
#include <vector>

class FineGrainedIntSet {
    Node* head;
    Node* tail;
    std::vector<Node*> garbage;
    std::mutex garbage_mtx;

public:
    FineGrainedIntSet() {
        head = new Node(INT_MIN);
        tail = new Node(INT_MAX);
        head->next = tail;
    }

    ~FineGrainedIntSet() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        for (auto* p : garbage) delete p;
    }

    FineGrainedIntSet(const FineGrainedIntSet&) = delete;
    FineGrainedIntSet& operator=(const FineGrainedIntSet&) = delete;

    bool add(int key) {
        head->mtx.lock();
        Node* prev = head;
        Node* curr = head->next;
        curr->mtx.lock();
        while (curr->key < key) {
            prev->mtx.unlock();
            prev = curr;
            curr = curr->next;
            curr->mtx.lock();
        }
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

    bool remove(int key) {
        head->mtx.lock();
        Node* prev = head;
        Node* curr = head->next;
        curr->mtx.lock();
        while (curr->key < key) {
            prev->mtx.unlock();
            prev = curr;
            curr = curr->next;
            curr->mtx.lock();
        }
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

    bool contains(int key) {
        head->mtx.lock();
        Node* prev = head;
        Node* curr = head->next;
        curr->mtx.lock();
        while (curr->key < key) {
            prev->mtx.unlock();
            prev = curr;
            curr = curr->next;
            curr->mtx.lock();
        }
        bool found = (curr->key == key);
        curr->mtx.unlock();
        prev->mtx.unlock();
        return found;
    }
};
