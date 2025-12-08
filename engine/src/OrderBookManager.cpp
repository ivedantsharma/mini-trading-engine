#include "OrderBookManager.hpp"
#include <iostream>

std::vector<Trade> OrderBookManager::addOrder(const std::string& symbol, const Order& order) {
    // Ensure book exists
    auto &book = books[symbol]; // default-construct if missing

    // Per-book trades (tradeIds might be local to that book)
    std::vector<Trade> localTrades = book.addOrder(order);

    // Remap their tradeId to a global sequence and return
    std::vector<Trade> out;
    out.reserve(localTrades.size());
    for (auto &t : localTrades) {
        Trade tt = t;
        tt.tradeId = globalTradeId++;
        out.push_back(tt);
    }
    return out;
}

void OrderBookManager::cancelOrder(const std::string& symbol, uint64_t orderId) {
    auto it = books.find(symbol);
    if (it == books.end()) {
        std::cout << "Symbol " << symbol << " not found\n";
        return;
    }
    it->second.cancelOrder(orderId);
}

void OrderBookManager::printTopLevels() const {
    if (books.empty()) {
        std::cout << "No symbols in manager\n";
        return;
    }
    for (const auto &p : books) {
        std::cout << "=== " << p.first << " ===\n";
        p.second.printTopLevels();
    }
}

void OrderBookManager::printTopLevels(const std::string& symbol) const {
    auto it = books.find(symbol);
    if (it == books.end()) {
        std::cout << "No book for symbol " << symbol << "\n";
        return;
    }
    std::cout << "=== " << symbol << " ===\n";
    it->second.printTopLevels();
}
