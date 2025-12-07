#pragma once
#include <string>
#include <cstdint>

struct Order {
    uint64_t orderId;
    std::string symbol;
    double price;
    uint32_t quantity;
    bool isBuy; // true = buy, false = sell
};
