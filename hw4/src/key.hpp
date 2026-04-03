#pragma once
#include <string>
#include <cstdint>
#include <compare>

class Key {
public:
    static constexpr char ALPHABET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static constexpr int ALPHA_SIZE = 62;
    static constexpr size_t MAX_LEN = 32;

    explicit Key(std::string value);

    const std::string& str() const;

    std::weak_ordering operator<=>(const Key& other) const;
    bool operator==(const Key& other) const;

    bool increment();
    Key advanced(uint64_t steps) const;

    static int char_index(char c);

private:
    std::string value_;
};
