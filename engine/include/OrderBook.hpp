#pragma once

#include <map>
#include <deque>
#include <vector>
#include <cstdint>
#include "Order.hpp"

struct Trade {
    uint64_t tradeId;
    uint64_t buyOrderId;
    uint64_t sellOrderId;
    double price;
    uint32_t quantity;
    uint64_t timestamp;
};

struct DepthLevel {
    double price;
    uint32_t size;
};

// Comparator: highest price first (for bids)
struct DescendingPrice {
    bool operator()(double a, double b) const {
        return a > b;       // higher prices come first
    }
};

class OrderBook {
public:
    OrderBook();

    // bids = highest price first
    std::map<double, std::deque<Order>, DescendingPrice> bids;

    // asks = lowest price first
    std::map<double, std::deque<Order>> asks;

    // Store mapping for cancellation
    std::map<uint64_t, double> orderIdToPrice;

    uint64_t nextTradeId = 1;

    std::vector<Trade> addOrder(const Order& order);

    // Return top `levels` depth as a vector of DepthLevel. If `isBid` is true,
    // returns bid-side levels (highest-first), otherwise ask-side (lowest-first).
    std::vector<DepthLevel> getDepth(bool isBid, int levels) const;

public:
    std::vector<Trade> matchLimitBuy(Order order);
    std::vector<Trade> matchLimitSell(Order order);

    std::vector<Trade> matchMarketBuy(Order order);
    std::vector<Trade> matchMarketSell(Order order);

    void insertLimitOrder(const Order& order);
    void cancelOrder(uint64_t orderId);

public:
    void printTopLevels() const;
};
