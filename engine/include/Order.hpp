#pragma once

#include <cstdint>
#include <string>

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    LIMIT,
    MARKET
};

struct Order {
    uint64_t orderId;
    std::string symbol;

    Side side;  
    OrderType type;

    // For LIMIT orders; ignored for MARKET orders
    double price;
    uint32_t quantity;

    // Nanoseconds since start or system clock
    uint64_t timestamp;
};
