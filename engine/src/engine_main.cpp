#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include "OrderBook.hpp"

// helper to get monotonic timestamp in nanoseconds
static uint64_t now_nanos() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

static void printTradeJSON(const Trade& t) {
    // simple JSON-line for readability
    std::cout << "{"
              << "\"tradeId\":" << t.tradeId << ","
              << "\"buyOrderId\":" << t.buyOrderId << ","
              << "\"sellOrderId\":" << t.sellOrderId << ","
              << "\"price\":" << t.price << ","
              << "\"quantity\":" << t.quantity << ","
              << "\"timestamp\":" << t.timestamp
              << "}" << std::endl;
}

static void printUsage() {
    std::cout << "Commands:\n"
              << "  NEW,<orderId>,<SYMBOL>,<BUY/SELL>,<LIMIT/MARKET>,<price or 0>,<qty>\n"
              << "    e.g. NEW,1,AAPL,BUY,LIMIT,100.5,10\n"
              << "    e.g. NEW,2,AAPL,SELL,MARKET,0,5\n"
              << "  CANCEL,<orderId>\n"
              << "  SNAP    -- prints top-level book\n"
              << "  QUIT\n";
}

int main() {
    OrderBook book;
    std::string line;
    std::cout << "Mini Trading Engine CLI (type HELP for usage)\n";

    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        // trim leading/trailing whitespace
        auto l = line;
        while (!l.empty() && isspace(l.back())) l.pop_back();

        if (l == "QUIT" || l == "EXIT") break;
        if (l == "HELP") { printUsage(); continue; }
        if (l == "SNAP") { book.printTopLevels(); continue; }

        // parse CSV-like command
        std::stringstream ss(l);
        std::string token;
        std::vector<std::string> parts;
        while (std::getline(ss, token, ',')) {
            // trim token
            size_t start = 0;
            while (start < token.size() && isspace((unsigned char)token[start])) ++start;
            size_t end = token.size();
            while (end > start && isspace((unsigned char)token[end-1])) --end;
            parts.push_back(token.substr(start, end-start));
        }

        if (parts.empty()) continue;

        const std::string cmd = parts[0];
        if (cmd == "NEW") {
            if (parts.size() != 7) {
                std::cerr << "NEW command requires 6 args. Type HELP.\n";
                continue;
            }

            // parse fields
            uint64_t orderId = std::stoull(parts[1]);
            std::string symbol = parts[2];
            std::string sideStr = parts[3];
            std::string typeStr = parts[4];
            double price = std::stod(parts[5]);
            uint32_t qty = static_cast<uint32_t>(std::stoul(parts[6]));

            Order o;
            o.orderId = orderId;
            o.symbol = symbol;
            o.side = (sideStr == "BUY" ? Side::BUY : Side::SELL);
            o.type = (typeStr == "LIMIT" ? OrderType::LIMIT : OrderType::MARKET);
            o.price = price;
            o.quantity = qty;
            o.timestamp = now_nanos();

            auto trades = book.addOrder(o);
            // print trades
            for (const auto& t : trades) {
                printTradeJSON(t);
            }
        } else if (cmd == "CANCEL") {
            if (parts.size() != 2) {
                std::cerr << "CANCEL requires orderId. Type HELP.\n";
                continue;
            }
            uint64_t orderId = std::stoull(parts[1]);
            book.cancelOrder(orderId);
        } else {
            std::cerr << "Unknown command. Type HELP for usage.\n";
        }
    }

    std::cout << "Exiting.\n";
    return 0;
}
