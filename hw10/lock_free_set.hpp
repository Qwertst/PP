#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

// #include <hazard_pointer> эх если бы..

template <typename T>
class LockFreeSet {
    static_assert(std::is_default_constructible_v<T>);

    struct Node;

    struct Link {
        uintptr_t w = 0;

        Link() = default;

        Link(Node *p, bool mark = false)
            : w(reinterpret_cast<uintptr_t>(p) | (mark ? 1 : 0)) {
        }

        Node *node() const {
            return reinterpret_cast<Node *>(w & ~uintptr_t(1));
        }

        bool marked() const {
            return (w & 1) != 0;
        }

        Link withMark() const {
            return Link(node(), true);
        }

        bool operator==(const Link &o) const {
            return w == o.w;
        }
    };

    struct Node {
        T key;
        std::atomic<Link> next;
        Node *retire_next = nullptr;

        explicit Node(const T &k) : key(k), next(Link{}) {
        }
    };

    static_assert(alignof(Node) >= 2);
    static_assert(std::atomic<Link>::is_always_lock_free);

    Node *const head_;
    Node *const tail_;
    std::atomic<Node *> retired_{nullptr};

    void retire(Node *n) {
        Node *old = retired_.load(std::memory_order_relaxed);
        do {
            n->retire_next = old;
        } while (!retired_.compare_exchange_weak(
            old, n, std::memory_order_release, std::memory_order_relaxed
        ));
    }

    bool find(const T &value, Node *&pred, Node *&curr) {
        while (true) {
            pred = head_;
            curr = pred->next.load(std::memory_order_acquire).node();
            while (true) {
                if (curr == tail_) {
                    return false;
                }
                Link succ = curr->next.load(std::memory_order_acquire);
                if (pred->next.load(std::memory_order_acquire) != Link(curr)) {
                    break;
                }

                if (succ.marked()) {
                    Link expected(curr);
                    if (!pred->next.compare_exchange_strong(
                            expected, Link(succ.node()),
                            std::memory_order_acq_rel, std::memory_order_acquire
                        )) {
                        break;
                    }
                    retire(curr);
                } else if (!(curr->key < value)) {
                    return !(value < curr->key);
                } else {
                    pred = curr;
                }
                curr = succ.node();
            }
        }
    }

public:
    LockFreeSet() : head_(new Node(T{})), tail_(new Node(T{})) {
        head_->next.store(Link(tail_), std::memory_order_relaxed);
        tail_->next.store(Link{}, std::memory_order_relaxed);
    }

    ~LockFreeSet() {
        for (Node *c = head_; c != nullptr;) {
            Node *nxt = c->next.load(std::memory_order_relaxed).node();
            delete c;
            c = nxt;
        }
        for (Node *r = retired_.load(std::memory_order_relaxed);
             r != nullptr;) {
            Node *nxt = r->retire_next;
            delete r;
            r = nxt;
        }
    }

    LockFreeSet(const LockFreeSet &) = delete;
    LockFreeSet &operator=(const LockFreeSet &) = delete;

    bool add(T value) {
        Node *node = new Node(value);
        while (true) {
            Node *pred, *curr;
            if (find(value, pred, curr)) {
                delete node;
                return false;
            }
            node->next.store(Link(curr), std::memory_order_relaxed);
            Link expected(curr);
            if (pred->next.compare_exchange_strong(
                    expected, Link(node), std::memory_order_release,
                    std::memory_order_acquire
                )) {
                return true;
            }
        }
    }

    bool remove(T value) {
        while (true) {
            Node *pred, *curr;
            if (!find(value, pred, curr)) {
                return false;
            }
            Link cn = curr->next.load(std::memory_order_acquire);
            if (cn.marked()) {
                continue;
            }
            if (curr->next.compare_exchange_strong(
                    cn, cn.withMark(), std::memory_order_acq_rel,
                    std::memory_order_acquire
                )) {
                find(value, pred, curr);
                return true;
            }
        }
    }

    bool contains(T value) const {
        Node *curr = head_->next.load(std::memory_order_acquire).node();
        while (curr != tail_ && curr->key < value) {
            curr = curr->next.load(std::memory_order_acquire).node();
        }
        if (curr == tail_) {
            return false;
        }
        Link cn = curr->next.load(std::memory_order_acquire);
        return !cn.marked() && !(value < curr->key);
    }

    bool isEmpty() const {
        Node *curr = head_->next.load(std::memory_order_acquire).node();
        while (curr != tail_) {
            Link cn = curr->next.load(std::memory_order_acquire);
            if (!cn.marked()) {
                return false;
            }
            curr = cn.node();
        }
        return true;
    }
};
