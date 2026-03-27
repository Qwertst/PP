#pragma once

#include "node.hpp"

class CoarseGrainedIntSet {
    Node* head;
    Node* tail;
    std::mutex mtx;

public:
    CoarseGrainedIntSet() {
        head = new Node(INT_MIN);
        tail = new Node(INT_MAX);
        head->next = tail;
    }

    ~CoarseGrainedIntSet() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
    }

    CoarseGrainedIntSet(const CoarseGrainedIntSet&) = delete;
    CoarseGrainedIntSet& operator=(const CoarseGrainedIntSet&) = delete;

    bool add(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        Node* prev = head;
        Node* curr = head->next;
        while (curr->key < key) {
            prev = curr;
            curr = curr->next;
        }
        if (curr->key == key) return false;
        Node* node = new Node(key);
        node->next = curr;
        prev->next = node;
        return true;
    }

    bool remove(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        Node* prev = head;
        Node* curr = head->next;
        while (curr->key < key) {
            prev = curr;
            curr = curr->next;
        }
        if (curr->key != key) return false;
        prev->next = curr->next;
        delete curr;
        return true;
    }

    bool contains(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        Node* curr = head->next;
        while (curr->key < key) {
            curr = curr->next;
        }
        return curr->key == key;
    }
};
