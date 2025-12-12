#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include "OrderBook.hpp"

// Small POD to store best bid/ask
struct TopOfBook {
    bool hasBid = false;
    bool hasAsk = false;
    double bestBid = 0.0;
    double bestAsk = 0.0;
};

class OrderBookManager {
    public:
        OrderBookManager() : globalTradeId(1) {}

        std::vector<Trade> addOrder(const std::string& symbol, const Order& order);
        void cancelOrder(const std::string& symbol, uint64_t orderId);

        void printTopLevels() const;                  // print all symbols
        void printTopLevels(const std::string& symbol) const; // print specific symbol

        OrderBook* getOrderBook(const std::string& symbol);

    private:
        std::map<std::string, OrderBook> books;
        std::map<std::string, TopOfBook> prevTop; // track previous top-of-book per symbol
        uint64_t globalTradeId;

        // helpers
        TopOfBook snapshotTop(const std::string& symbol) const;
        void emitMarketDataTop(const std::string& symbol, const TopOfBook& top) const;
        void emitTradeMD(const Trade& t, const std::string& symbol) const;
};
