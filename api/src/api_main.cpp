#include <iostream>
#include <thread>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "../../engine/include/OrderBook.hpp"

using json = nlohmann::json;
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
        }
    }
}

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, {tcp::v4(), 9001});

        std::cout << "WebSocket API listening on ws://localhost:9001\n";

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
