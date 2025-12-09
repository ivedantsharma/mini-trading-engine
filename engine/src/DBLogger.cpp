#include "DBLogger.hpp"
#include <iostream>

DBLogger::DBLogger() : db(nullptr) {}

DBLogger::~DBLogger() {
    if (db) sqlite3_close(db);
}

bool DBLogger::init(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[DB] Failed to open DB: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    exec("CREATE TABLE IF NOT EXISTS Orders ("
         "orderId INTEGER, symbol TEXT, side TEXT, type TEXT,"
         "price REAL, quantity INTEGER, timestamp INTEGER);");

    exec("CREATE TABLE IF NOT EXISTS Trades ("
         "tradeId INTEGER, symbol TEXT, price REAL, quantity INTEGER,"
         "buyOrderId INTEGER, sellOrderId INTEGER, timestamp INTEGER);");

    std::cerr << "[DB] Initialized successfully\n";
    return true;
}

void DBLogger::exec(const std::string& sql) {
    char* errMsg = nullptr;
    std::lock_guard<std::mutex> lock(mtx);

    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "[DB] SQL Error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}

void DBLogger::logOrder(const Order& o) {
    std::string side = (o.side == Side::BUY ? "BUY" : "SELL");
    std::string type = (o.type == OrderType::LIMIT ? "LIMIT" : "MARKET");

    std::string sql =
        "INSERT INTO Orders VALUES (" +
        std::to_string(o.orderId) + ", '" + o.symbol + "', '" + side +
        "', '" + type + "', " + std::to_string(o.price) + ", " +
        std::to_string(o.quantity) + ", " + std::to_string(o.timestamp) + ");";

    exec(sql);
}

void DBLogger::logTrade(const Trade& t, const std::string& symbol) {
    std::string sql =
        "INSERT INTO Trades VALUES (" +
        std::to_string(t.tradeId) + ", '" + symbol + "', " +
        std::to_string(t.price) + ", " + std::to_string(t.quantity) + ", " +
        std::to_string(t.buyOrderId) + ", " +
        std::to_string(t.sellOrderId) + ", " +
        std::to_string(t.timestamp) + ");";

    exec(sql);
}
