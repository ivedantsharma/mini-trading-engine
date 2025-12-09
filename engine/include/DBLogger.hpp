#pragma once
#include <sqlite3.h>
#include <string>
#include <mutex>
#include "Order.hpp"
#include "OrderBook.hpp"  // for Trade struct

class DBLogger {
public:
    DBLogger();
    ~DBLogger();

    bool init(const std::string& path);

    void logOrder(const Order& o);
    void logTrade(const Trade& t, const std::string& symbol);

private:
    sqlite3* db;
    std::mutex mtx;

    void exec(const std::string& sql);
};
