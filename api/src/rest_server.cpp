#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../include/httplib.h"

using json = nlohmann::json;

json getTrades(const std::string &symbol, int limit) {
    sqlite3* db;
    sqlite3_open("trading.db", &db);

    std::string sql =
        "SELECT tradeId, buyOrderId, sellOrderId, price, quantity, timestamp "
        "FROM Trades WHERE symbol=? ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

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

void start_rest_server() {
    httplib::Server svr;

    // ---- TRADES ----
    svr.Get("/trades", [](const httplib::Request& req, httplib::Response& res) {
        std::string symbol = req.get_param_value("symbol");
        std::string limit_str = req.get_param_value("limit");
        int limit = limit_str.empty() ? 100 : std::stoi(limit_str);

        json data = getTrades(symbol, limit);
        res.set_content(data.dump(), "application/json");
    });

    std::cout << "[REST] Listening on http://localhost:9003\n";
    svr.listen("0.0.0.0", 9003);
}
