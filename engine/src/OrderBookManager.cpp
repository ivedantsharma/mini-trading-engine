#include "MarketDataServer.hpp"
#include "OrderBookManager.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include "DBLogger.hpp"

static DBLogger DB;

// helper timestamp (ns)
static uint64_t now_nanos() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<std::chrono::nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

std::vector<Trade> OrderBookManager::addOrder(const std::string& symbol, const Order& order) {
    // Log the incoming order
    DB.logOrder(order);
    
    // Ensure book exists
    auto &book = books[symbol]; // default-construct if missing

    // Snapshot before
    TopOfBook before = snapshotTop(symbol);

    // Per-book trades (tradeIds might be local to that book)
    std::vector<Trade> localTrades = book.addOrder(order);

    // Remap their tradeId to a global sequence and return
    std::vector<Trade> out;
    out.reserve(localTrades.size());
    for (auto &t : localTrades) {
        Trade tt = t;
        tt.tradeId = globalTradeId++;
        out.push_back(tt);

        // Emit trade market-data line as well (to stderr)
        emitTradeMD(tt, symbol);
    }

    // Snapshot after
    TopOfBook after = snapshotTop(symbol);

    // If top-of-book changed, emit an update
    bool changed = false;
    if (before.hasBid != after.hasBid) changed = true;
    else if (before.hasAsk != after.hasAsk) changed = true;
    else if (before.hasBid && after.hasBid && before.bestBid != after.bestBid) changed = true;
    else if (before.hasAsk && after.hasAsk && before.bestAsk != after.bestAsk) changed = true;

    if (changed) {
        prevTop[symbol] = after;
        emitMarketDataTop(symbol, after);
    }

    return out;
}

void OrderBookManager::cancelOrder(const std::string& symbol, uint64_t orderId) {
    auto it = books.find(symbol);
    if (it == books.end()) {
        std::cout << "Symbol " << symbol << " not found\n";
        return;
    }
    // snapshot before
    TopOfBook before = snapshotTop(symbol);

    it->second.cancelOrder(orderId);

    TopOfBook after = snapshotTop(symbol);
    bool changed = false;
    if (before.hasBid != after.hasBid) changed = true;
    else if (before.hasAsk != after.hasAsk) changed = true;
    else if (before.hasBid && after.hasBid && before.bestBid != after.bestBid) changed = true;
    else if (before.hasAsk && after.hasAsk && before.bestAsk != after.bestAsk) changed = true;

    if (changed) {
        prevTop[symbol] = after;
        emitMarketDataTop(symbol, after);
    }
}

OrderBook* OrderBookManager::getOrderBook(const std::string& symbol) {
    auto it = books.find(symbol);
    if (it == books.end()) return nullptr;
    return &it->second;
}

void OrderBookManager::printTopLevels() const {
    if (books.empty()) {
        std::cout << "No orderbooks yet.\n";
        return;
    }
    for (auto& kv : books) {
        std::cout << "=== " << kv.first << " ===\n";
        kv.second.printTopLevels();
    }
}

void OrderBookManager::printTopLevels(const std::string& symbol) const {
    auto it = books.find(symbol);
    if (it == books.end()) {
        std::cout << "No book for " << symbol << "\n";
        return;
    }
    std::cout << "=== " << symbol << " ===\n";
    it->second.printTopLevels();
}

TopOfBook OrderBookManager::snapshotTop(const std::string& symbol) const {
    TopOfBook res;
    auto it = books.find(symbol);
    if (it == books.end()) return res;

    const auto& book = it->second;

    // For bids (descending map) begin() is best
    if (!book.bids.empty()) {
        res.hasBid = true;
        res.bestBid = book.bids.begin()->first;
    } else {
        res.hasBid = false;
    }

    // For asks (ascending map) begin() is best ask
    if (!book.asks.empty()) {
        res.hasAsk = true;
        res.bestAsk = book.asks.begin()->first;
    } else {
        res.hasAsk = false;
    }

    return res;
}

void OrderBookManager::emitMarketDataTop(const std::string& symbol, const TopOfBook& top) const {
    // Emit JSON line to stderr so it's separable from stdout (trades)
    // Example:
    // {"type":"top","symbol":"AAPL","bestBid":100.5,"bestAsk":100.6,"timestamp":12345}
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{\"type\":\"top\",\"symbol\":\"" << symbol << "\"";

    if (top.hasBid) ss << ",\"bestBid\":" << top.bestBid;
    else ss << ",\"bestBid\":null";

    if (top.hasAsk) ss << ",\"bestAsk\":" << top.bestAsk;
    else ss << ",\"bestAsk\":null";

    ss << ",\"timestamp\":" << now_nanos();
    ss << "}\n";
    MarketDataServerAPI::broadcast(ss.str());
}

void OrderBookManager::emitTradeMD(const Trade& t, const std::string& symbol) const {
    // Emit trade as market-data JSON line to stderr as well, helpful for dashboard
    // {"type":"trade","symbol":"AAPL","tradeId":..., "price":..., "quantity":..., "buyOrderId":..., "sellOrderId":..., "timestamp":...}
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{\"type\":\"trade\",\"symbol\":\"" << symbol << "\","
       << "\"tradeId\":" << t.tradeId << ","
       << "\"price\":" << t.price << ","
       << "\"quantity\":" << t.quantity << ","
       << "\"buyOrderId\":" << t.buyOrderId << ","
       << "\"sellOrderId\":" << t.sellOrderId << ","
       << "\"timestamp\":" << t.timestamp
       << "}\n";
    MarketDataServerAPI::broadcast(ss.str());
}