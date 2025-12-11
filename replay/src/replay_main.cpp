#include <iostream>
#include <thread>
#include <chrono>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../../engine/include/Order.hpp"
#include "../../engine/include/OrderBook.hpp"
#include "../../engine/include/MarketDataServer.hpp"

using json = nlohmann::json;

struct ReplayOrder {
    uint64_t orderId;
    std::string symbol;
    std::string side;
    std::string type;
    double price;
    uint32_t qty;
    uint64_t ts;
};

struct ReplayTrade {
    uint64_t tradeId;
    std::string symbol;
    double price;
    uint32_t qty;
    uint64_t ts;
};

static sqlite3* db = nullptr;

static void fail(const std::string& msg) {
    std::cerr << "[Replay] Error: " << msg << std::endl;
    exit(1);
}

std::vector<ReplayTrade> loadTrades() {
    std::vector<ReplayTrade> v;
    sqlite3_stmt* stmt;

    const char* q =
        "SELECT tradeId, symbol, price, quantity, timestamp "
        "FROM Trades ORDER BY timestamp ASC;";

    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) != SQLITE_OK)
        fail("Cannot run query Trades");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ReplayTrade t;
        t.tradeId = sqlite3_column_int64(stmt, 0);
        t.symbol = (const char*)sqlite3_column_text(stmt, 1);
        t.price = sqlite3_column_double(stmt, 2);
        t.qty = sqlite3_column_int(stmt, 3);
        t.ts = sqlite3_column_int64(stmt, 4);
        v.push_back(t);
    }

    sqlite3_finalize(stmt);
    return v;
}

int main() {
    std::cout << "[Replay] Starting replay from DB..." << std::endl;

    if (sqlite3_open("trading.db", &db) != SQLITE_OK)
        fail("Cannot open trading.db");

    auto trades = loadTrades();
    std::cout << "[Replay] Loaded " << trades.size() << " trades." << std::endl;

    // Start market-data WS server
    unsigned short port = 9100;
    MarketDataServerAPI::start(port);
    std::cout << "[Replay] WS on ws://localhost:" << port << std::endl;

    // Loop through historical trades
    for (auto &t : trades) {
        json msg = {
            {"type", "trade"},
            {"symbol", t.symbol},
            {"tradeId", t.tradeId},
            {"price", t.price},
            {"quantity", t.qty},
            {"timestamp", t.ts}
        };

        MarketDataServerAPI::broadcast(msg.dump());

        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // playback speed
    }

    std::cout << "[Replay] Completed." << std::endl;

    MarketDataServerAPI::stop();
    sqlite3_close(db);
    return 0;
}
