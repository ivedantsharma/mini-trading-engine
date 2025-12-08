#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include "OrderBook.hpp"

// Manager that maintains per-symbol orderbooks and a global trade id
class OrderBookManager {
public:
    OrderBookManager() : globalTradeId(1) {}

    // Route order to per-symbol book and retag trade IDs to be globally unique
    std::vector<Trade> addOrder(const std::string& symbol, const Order& order);

    // Cancel an order in a specific symbol
    void cancelOrder(const std::string& symbol, uint64_t orderId);

    // Print top levels for all symbols or single symbol if provided
    void printTopLevels() const;
    void printTopLevels(const std::string& symbol) const;

private:
    std::map<std::string, OrderBook> books;
    uint64_t globalTradeId;
};
