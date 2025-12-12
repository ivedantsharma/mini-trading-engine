#include <iostream>
#include <thread>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

#include "../../engine/include/OrderBook.hpp"
#include "../../engine/include/OrderBookManager.hpp"

using json = nlohmann::json;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Forward declarations
void start_rest_server();
json fetchTradesForReplay(const std::string& symbol, uint64_t ts_from, uint64_t ts_to);

static OrderBookManager mgr;
static uint64_t nextOrderId = 1000;

// Keep list of connected WS clients (we broadcast all messages to every client; client filters by symbol)
static std::vector<std::shared_ptr<websocket::stream<tcp::socket>>> clients;

// -------------------- SQLite helpers --------------------
static bool ensure_db_schema() {
    sqlite3* db = nullptr;
    if (sqlite3_open("trading.db", &db) != SQLITE_OK) {
        std::cerr << "[DB] failed to open DB\n";
        if (db) sqlite3_close(db);
        return false;
    }

    const char *createTrades =
        "CREATE TABLE IF NOT EXISTS Trades ("
        " tradeId INTEGER PRIMARY KEY,"
        " symbol TEXT,"
        " price REAL,"
        " quantity INTEGER,"
        " buyOrderId INTEGER,"
        " sellOrderId INTEGER,"
        " timestamp INTEGER"
        ");";

    const char *createCandles =
        "CREATE TABLE IF NOT EXISTS Candles ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " symbol TEXT,"
        " tf INTEGER,"
        " start_ts INTEGER,"
        " open REAL,"
        " high REAL,"
        " low REAL,"
        " close REAL,"
        " volume INTEGER"
        ");";

    char* err = nullptr;
    if (sqlite3_exec(db, createTrades, nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "[DB] create Trades failed: " << (err ? err : "") << "\n";
        sqlite3_free(err);
        sqlite3_close(db);
        return false;
    }
    if (sqlite3_exec(db, createCandles, nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "[DB] create Candles failed: " << (err ? err : "") << "\n";
        sqlite3_free(err);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

/* Save a trade row to DB */
static void saveTradeToDB(const Trade &t, const std::string &symbol) {
    sqlite3* db = nullptr;
    if (sqlite3_open("trading.db", &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return;
    }

    const char *ins =
        "INSERT OR REPLACE INTO Trades (tradeId, symbol, price, quantity, buyOrderId, sellOrderId, timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, ins, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, (sqlite3_int64)t.tradeId);
        sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 3, t.price);
        sqlite3_bind_int(stmt, 4, (int)t.quantity);
        sqlite3_bind_int64(stmt, 5, (sqlite3_int64)t.buyOrderId);
        sqlite3_bind_int64(stmt, 6, (sqlite3_int64)t.sellOrderId);
        sqlite3_bind_int64(stmt, 7, (sqlite3_int64)t.timestamp);
        sqlite3_step(stmt);
    }
    if (stmt) sqlite3_finalize(stmt);
    sqlite3_close(db);
}

static uint64_t candle_start_for_tf(uint64_t trade_ts, int tf_seconds) {
    uint64_t tf_nsec = (uint64_t)tf_seconds * 1000000000ULL;
    return (trade_ts / tf_nsec) * tf_nsec;
}

static void upsertCandle(const std::string &symbol, int tf_seconds, uint64_t start_ts, double price, uint32_t qty) {
    sqlite3* db = nullptr;
    if (sqlite3_open("trading.db", &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return;
    }

    const char* update_sql =
        "UPDATE Candles SET "
        " high = CASE WHEN ? > high THEN ? ELSE high END, "
        " low  = CASE WHEN ? < low  THEN ? ELSE low  END, "
        " close = ?, "
        " volume = volume + ? "
        "WHERE symbol = ? AND tf = ? AND start_ts = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, price);
        sqlite3_bind_double(stmt, 2, price);
        sqlite3_bind_double(stmt, 3, price);
        sqlite3_bind_double(stmt, 4, price);
        sqlite3_bind_double(stmt, 5, price); // close
        sqlite3_bind_int(stmt, 6, (int)qty);
        sqlite3_bind_text(stmt, 7, symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 8, tf_seconds);
        sqlite3_bind_int64(stmt, 9, (sqlite3_int64)start_ts);
        sqlite3_step(stmt);
    }
    if (stmt) sqlite3_finalize(stmt);

    bool updated = (sqlite3_changes(db) > 0);

    if (!updated) {
        const char* insert_sql =
            "INSERT INTO Candles (symbol, tf, start_ts, open, high, low, close, volume) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* ins = nullptr;
        if (sqlite3_prepare_v2(db, insert_sql, -1, &ins, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(ins, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(ins, 2, tf_seconds);
            sqlite3_bind_int64(ins, 3, (sqlite3_int64)start_ts);
            sqlite3_bind_double(ins, 4, price); // open
            sqlite3_bind_double(ins, 5, price); // high
            sqlite3_bind_double(ins, 6, price); // low
            sqlite3_bind_double(ins, 7, price); // close
            sqlite3_bind_int(ins, 8, (int)qty);
            sqlite3_step(ins);
        }
        if (ins) sqlite3_finalize(ins);
    }

    sqlite3_close(db);
}

static void updateCandlesOnTrade(const Trade &t, const std::string &symbol) {
    const int tfs[] = {1, 60};
    for (int tf : tfs) {
        uint64_t start = candle_start_for_tf(t.timestamp, tf);
        upsertCandle(symbol, tf, start, t.price, t.quantity);
    }
}

// Broadcast to every WS client (clients should filter by symbol)
void broadcast(const json& j) {
    std::string msg = j.dump();
    for (auto it = clients.begin(); it != clients.end();) {
        auto& ws = *it;
        beast::error_code ec;
        ws->text(true);
        ws->write(net::buffer(msg), ec);
        if (ec) {
            // drop broken connection
            it = clients.erase(it);
        } else ++it;
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
    auto* book = mgr.getOrderBook(symbol);
    if (!book) return;

    auto bids = book->getDepth(true, 10);
    auto asks = book->getDepth(false, 10);

    json j;
    j["type"] = "top";
    j["symbol"] = symbol;
    j["timestamp"] = (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();

    j["bids"] = json::array();
    j["asks"] = json::array();

    for (auto& b : bids)
        j["bids"].push_back({{"price", b.price}, {"qty", b.size}});

    for (auto& a : asks)
        j["asks"].push_back({{"price", a.price}, {"qty", a.size}});

    if (!bids.empty())
        j["bestBid"] = bids.front().price;
    else
        j["bestBid"] = nullptr;

    if (!asks.empty())
        j["bestAsk"] = asks.front().price;
    else
        j["bestAsk"] = nullptr;

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

    // Add to the manager (per-symbol book)
    auto trades = mgr.addOrder(ord.symbol, ord);

    // Persist & broadcast each trade
    for (auto &t : trades) {
        saveTradeToDB(t, ord.symbol);
        updateCandlesOnTrade(t, ord.symbol);
        broadcast(tradeToJSON(t, ord.symbol));
    }

    // broadcast top-of-book for that symbol
    broadcastTop(ord.symbol);
}

void handleCancel(const json& message) {
    std::string symbol = message.value("symbol", "");
    uint64_t id = message.value("orderId", (uint64_t)0);

    if (symbol.empty()) {
        std::cerr << "[WARN] CANCEL missing symbol\n";
        return;
    }

    mgr.cancelOrder(symbol, id);
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

        std::string cmd = j.value("cmd", std::string());
        if (cmd == "NEW") {
            handleNewOrder(j);
        } else if (cmd == "CANCEL") {
            handleCancel(j);
        } else if (cmd == "REPLAY") {
            std::string symbol = j["symbol"];
            uint64_t from = j["from"];
            uint64_t to = j["to"];

            json result = {
                {"type", "replayData"},
                {"symbol", symbol},
                {"trades", fetchTradesForReplay(symbol, from, to)}
            };

            ws->text(true);
            ws->write(net::buffer(result.dump()));
        }
    }
}

json fetchTradesForReplay(const std::string& symbol, uint64_t ts_from, uint64_t ts_to) {
    sqlite3* db;
    if (sqlite3_open("trading.db", &db) != SQLITE_OK) return json::array();

    std::string sql =
        "SELECT tradeId, buyOrderId, sellOrderId, price, quantity, timestamp "
        "FROM Trades WHERE symbol=? AND timestamp BETWEEN ? AND ? ORDER BY timestamp ASC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return json::array();
    }

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)ts_from);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)ts_to);

    json arr = json::array();

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        json row = {
            {"tradeId", sqlite3_column_int64(stmt, 0)},
            {"buyOrderId", sqlite3_column_int64(stmt, 1)},
            {"sellOrderId", sqlite3_column_int64(stmt, 2)},
            {"price", sqlite3_column_double(stmt, 3)},
            {"quantity", sqlite3_column_int(stmt, 4)},
            {"timestamp", sqlite3_column_int64(stmt, 5)}
        };
        arr.push_back(row);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return arr;
}

int main() {
    // ensure DB schema
    if (!ensure_db_schema()) {
        std::cerr << "[API] DB initialization failed — exiting\n";
        return 1;
    }

    try {
        // start REST server first (background)
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

    return 0;
}
