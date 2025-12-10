// api/src/api_main.cpp
#include <iostream>
#include <thread>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "../../engine/include/OrderBook.hpp"
#include <sqlite3.h>

// forward
void start_rest_server();

using json = nlohmann::json;
json fetchTradesForReplay(const std::string& symbol, uint64_t ts_from, uint64_t ts_to);

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

OrderBook book;
uint64_t nextOrderId = 1000;
std::vector<std::shared_ptr<websocket::stream<tcp::socket>>> clients;

void broadcast(const json& j) {
    std::string msg = j.dump();
    for (auto& ws : clients) {
        beast::error_code ec;
        ws->text(true);
        ws->write(net::buffer(msg), ec);
    }
}

json tradeToJSON(const Trade& t, const std::string& symbol) {
    return json{
        {"type", "trade"},
        {"symbol", symbol},
        {"tradeId", t.tradeId},
        {"price", t.price},
        {"quantity", t.quantity},
        {"buyOrderId", t.buyOrderId},
        {"sellOrderId", t.sellOrderId},
        {"timestamp", t.timestamp}
    };
}

void broadcastTop(const std::string& symbol) {
    double bestBid = book.bids.empty() ? 0.0 : book.bids.begin()->first;
    double bestAsk = book.asks.empty() ? 0.0 : book.asks.begin()->first;

    // Add depth snapshot (top 10)
    auto bidLevels = book.getDepth(true, 10);
    auto askLevels = book.getDepth(false, 10);

    json bids_json = json::array();
    json asks_json = json::array();

    for (auto &lvl : bidLevels) {
        bids_json.push_back({ {"price", lvl.price}, {"qty", lvl.size} });
    }
    for (auto &lvl : askLevels) {
        asks_json.push_back({ {"price", lvl.price}, {"qty", lvl.size} });
    }

    json j = {
        {"type", "top"},
        {"symbol", symbol},
        {"bestBid", book.bids.empty() ? nullptr : json(bestBid)},
        {"bestAsk", book.asks.empty() ? nullptr : json(bestAsk)},
        {"timestamp", (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count()}
    };

    j["bids"] = bids_json;
    j["asks"] = asks_json;

    broadcast(j);
}

void handleNewOrder(const json& message) {
    auto o = message["order"];

    Order ord;
    ord.orderId = nextOrderId++;
    ord.symbol = o["symbol"];
    ord.side = (o["side"] == "BUY" ? Side::BUY : Side::SELL);
    ord.type = (o["type"] == "LIMIT" ? OrderType::LIMIT : OrderType::MARKET);
    ord.price = o["price"];
    ord.quantity = o["quantity"];
    ord.timestamp = (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count();

    auto trades = book.addOrder(ord);

    for (auto& t : trades) {
        broadcast(tradeToJSON(t, ord.symbol));
    }

    broadcastTop(ord.symbol);
}

void handleCancel(const json& message) {
    uint64_t id = message["orderId"];
    book.cancelOrder(id);
}

void session(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
    ws->accept();
    std::cout << "[API] Client connected\n";
    clients.push_back(ws);

    beast::flat_buffer buffer;

    while (true) {
        beast::error_code ec;
        ws->read(buffer, ec);
        if (ec) break;

        std::string msg = beast::buffers_to_string(buffer.data());
        buffer.consume(buffer.size());

        auto j = json::parse(msg, nullptr, false);
        if (j.is_discarded()) continue;

        if (j["cmd"] == "NEW") {
            handleNewOrder(j);
        } else if (j["cmd"] == "CANCEL") {
            handleCancel(j);
        } else if (j["cmd"] == "REPLAY") {
            std::string symbol = j["symbol"];
            uint64_t from = j["from"];
            uint64_t to = j["to"];

            json result = {
                {"type", "replayData"},
                {"symbol", symbol},
                {"trades", json::array()}
            };

            // call fetchTradesForReplay to fill trades
            result["trades"] = fetchTradesForReplay(symbol, from, to);

            ws->text(true);
            ws->write(net::buffer(result.dump()));
        }
    }
}

json fetchTradesForReplay(const std::string& symbol, uint64_t ts_from, uint64_t ts_to) {
    sqlite3* db;
    if (sqlite3_open("trading.db", &db) != SQLITE_OK) {
        sqlite3_close(db);
        return json::array();
    }

    std::string sql =
        "SELECT tradeId, buyOrderId, sellOrderId, price, quantity, timestamp "
        "FROM Trades WHERE symbol=? AND timestamp BETWEEN ? AND ? ORDER BY timestamp ASC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return json::array();
    }

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, ts_from);
    sqlite3_bind_int64(stmt, 3, ts_to);

    json arr = json::array();

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        json row = {
            {"tradeId", sqlite3_column_int64(stmt, 0)},
            {"buyOrderId", sqlite3_column_int64(stmt, 1)},
            {"sellOrderId", sqlite3_column_int64(stmt, 2)},
            {"price", sqlite3_column_double(stmt, 3)},
            {"quantity", sqlite3_column_int(stmt, 4)},
            {"timestamp", sqlite3_column_int64(stmt, 5)},
            {"symbol", symbol},
            {"type", "trade"}
        };
        arr.push_back(row);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return arr;
}

int main() {
    try {
        // Start REST server first (so it is available even while WS accept loop runs)
        std::thread restThread(start_rest_server);
        restThread.detach();

        net::io_context ioc;
        tcp::acceptor acceptor(ioc, {tcp::v4(), 9001});

        std::cout << "WebSocket API listening on ws://localhost:9001\n";
        std::cout << "[REST] (started in background)\n";

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);

            auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
            std::thread(session, ws).detach();
        }

    } catch (std::exception& e) {
        std::cerr << "API Server Error: " << e.what() << "\n";
    }

    // unreachable normally because the accept loop is infinite; main will end on exception or signal
    return 0;
}
