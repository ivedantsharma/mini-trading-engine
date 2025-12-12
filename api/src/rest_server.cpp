#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../include/httplib.h"
#include "Positions.hpp"

using json = nlohmann::json;

/* Query recent trades for symbol */
json getTrades(const std::string &symbol, int limit) {
    sqlite3* db;
    if (sqlite3_open("trading.db", &db) != SQLITE_OK) return json::array();

    std::string sql =
        "SELECT tradeId, buyOrderId, sellOrderId, price, quantity, timestamp "
        "FROM Trades WHERE symbol=? ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return json::array();
    }

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, limit);

    json arr = json::array();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        arr.push_back({
            {"tradeId", sqlite3_column_int64(stmt, 0)},
            {"buyOrderId", sqlite3_column_int64(stmt, 1)},
            {"sellOrderId", sqlite3_column_int64(stmt, 2)},
            {"price", sqlite3_column_double(stmt, 3)},
            {"quantity", sqlite3_column_int(stmt, 4)},
            {"timestamp", sqlite3_column_int64(stmt, 5)}
        });
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return arr;
}

/* Query candles stored in Candles table.
   tf param is seconds (e.g., 1, 60, 300)
*/
json getCandles(const std::string &symbol, int tf, int limit) {
    sqlite3* db;
    if (sqlite3_open("trading.db", &db) != SQLITE_OK) return json::array();

    std::string sql =
        "SELECT start_ts, open, high, low, close, volume "
        "FROM Candles WHERE symbol = ? AND tf = ? ORDER BY start_ts DESC LIMIT ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return json::array();
    }

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, tf);
    sqlite3_bind_int(stmt, 3, limit);

    json arr = json::array();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t start_ts   = sqlite3_column_int64(stmt, 0);
        double open        = sqlite3_column_double(stmt, 1);
        double high        = sqlite3_column_double(stmt, 2);
        double low         = sqlite3_column_double(stmt, 3);
        double close       = sqlite3_column_double(stmt, 4);
        int64_t volume     = sqlite3_column_int64(stmt, 5);

        // If column doesn't exist (old DB), updated_ts = start_ts
        int64_t updated_ts = 0;
        int col = sqlite3_column_count(stmt);
        if (col >= 7) {
            updated_ts = sqlite3_column_int64(stmt, 6);
        } else {
            updated_ts = start_ts;
        }

        arr.push_back({
            {"start_ts",  start_ts},
            {"open",      open},
            {"high",      high},
            {"low",       low},
            {"close",     close},
            {"volume",    volume},
            {"updated_ts", updated_ts},
            {"updateLatencyNs", updated_ts - start_ts}
        });
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return arr;
}

void start_rest_server() {
    httplib::Server svr;

    // GET /trades?symbol=AAPL&limit=50
    svr.Get("/trades", [](const httplib::Request& req, httplib::Response& res) {
        std::string symbol = req.get_param_value("symbol");
        std::string limit_str = req.has_param("limit") ? req.get_param_value("limit") : "100";
        
        if (symbol.empty()) {
            res.status = 400;
            res.set_content(R"({"error":"symbol parameter required"})", "application/json");
            return;
        }
        
        int limit = std::stoi(limit_str);
        json data = getTrades(symbol, limit);
        res.set_content(data.dump(), "application/json");
    });

    // GET /candles?symbol=AAPL&tf=60&limit=200
    svr.Get("/candles", [](const httplib::Request& req, httplib::Response& res) {
        std::string symbol = req.get_param_value("symbol");
        std::string tf_str = req.has_param("tf") ? req.get_param_value("tf") : "60";
        std::string limit_str = req.has_param("limit") ? req.get_param_value("limit") : "200";
        
        if (symbol.empty()) {
            res.status = 400;
            res.set_content(R"({"error":"symbol parameter required"})", "application/json");
            return;
        }
        
        int tf = std::stoi(tf_str);
        int limit = std::stoi(limit_str);
        json data = getCandles(symbol, tf, limit);
        res.set_content(data.dump(), "application/json");
    });

    // GET /positions
    svr.Get("/positions", [](const httplib::Request& req, httplib::Response& res) {
        auto snap = snapshot_positions();
        nlohmann::json arr = nlohmann::json::array();
        for (auto &kv : snap) {
            arr.push_back({
                {"symbol", kv.first},
                {"qty", kv.second.qty},
                {"avgPrice", kv.second.avgPrice},
                {"realizedPnl", kv.second.realizedPnl}
            });
        }
        res.set_content(arr.dump(), "application/json");
    });

    std::cout << "[REST] Listening on http://localhost:9003\n";
    svr.listen("0.0.0.0", 9003);
}
