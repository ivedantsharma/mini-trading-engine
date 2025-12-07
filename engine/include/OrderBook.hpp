#pragma once

#include "Order.hpp"
#include <map>
#include <deque>
#include <unordered_map>
#include <vector>

struct Trade {
    uint64_t tradeId;
    uint64_t buyOrderId;
    uint64_t sellOrderId;
    double price;
    uint32_t quantity;
    uint64_t timestamp;
};

class OrderBook {
public:
    OrderBook();

    std::vector<Trade> addOrder(const Order& order);  
    void cancelOrder(uint64_t orderId);
    void printTopLevels() const;

private:
    // BUY side → highest price first
    std::map<double, std::deque<Order>, std::greater<double>> bids;

    // SELL side → lowest price first
    std::map<double, std::deque<Order>, std::less<double>> asks;

    std::unordered_map<uint64_t, double> orderIdToPrice;

    uint64_t nextTradeId = 1;

    // Limit order matchers
    std::vector<Trade> matchLimitBuy(Order order);
    std::vector<Trade> matchLimitSell(Order order);

    // Market order matchers
    std::vector<Trade> matchMarketBuy(Order order);
    std::vector<Trade> matchMarketSell(Order order);

    // Helpers
    void insertLimitOrder(const Order& order);
};
