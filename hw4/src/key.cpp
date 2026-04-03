#include "key.hpp"

int Key::char_index(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 36 + (c - 'a');
    return -1;
}

Key::Key(std::string value) : value_(std::move(value)) {}

const std::string& Key::str() const {
    return value_;
}

std::weak_ordering Key::operator<=>(const Key& other) const {
    if (value_.size() != other.value_.size()) {
        return value_.size() < other.value_.size()
            ? std::weak_ordering::less
            : std::weak_ordering::greater;
    }
    if (value_ < other.value_) return std::weak_ordering::less;
    if (value_ > other.value_) return std::weak_ordering::greater;
    return std::weak_ordering::equivalent;
}

bool Key::operator==(const Key& other) const {
    return value_ == other.value_;
}

bool Key::increment() {
    int i = static_cast<int>(value_.size()) - 1;
    while (i >= 0) {
        int idx = char_index(value_[i]);
        if (idx < ALPHA_SIZE - 1) {
            value_[i] = ALPHABET[idx + 1];
            return true;
        }
        value_[i] = ALPHABET[0];
        --i;
    }
    if (value_.size() < MAX_LEN) {
        value_ = std::string(value_.size() + 1, ALPHABET[0]);
        return true;
    }
    return false;
}

Key Key::advanced(uint64_t steps) const {
    Key copy(value_);
    for (uint64_t i = 0; i < steps; ++i) {
        if (!copy.increment()) break;
    }
    return copy;
}
