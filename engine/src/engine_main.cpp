#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include "OrderBook.hpp"
#include <unistd.h>

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

static inline std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (unsigned char)s[start] <= 32) start++;

    size_t end = s.size();
    while (end > start && (unsigned char)s[end - 1] <= 32) end--;

    return s.substr(start, end - start);
}

int main() {
    OrderBook book;
    std::string line;

    std::cout << "Mini Trading Engine CLI (type HELP for usage)\n";

    while (true) {
        // Only show prompt in interactive terminal, NOT for redirected files
        if (isatty(fileno(stdin))) {
            std::cout << "> " << std::flush;
        }

        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        // ignore full-line comments quickly
        if (line[0] == '#') continue;

        // remove inline comments
        auto pos = line.find('#');
        if (pos != std::string::npos) {
            line = line.substr(0, pos);
        }

        // trim and skip empty lines after removing comments
        line = trim(line);
        if (line.empty()) continue;

        if (line == "QUIT" || line == "EXIT") break;
        if (line == "HELP") { printUsage(); continue; }
        if (line == "SNAP") { book.printTopLevels(); continue; }

        // Parse CSV tokens
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> parts;

        while (std::getline(ss, token, ',')) {
            parts.push_back(trim(token));   // trim EVERY token
        }

        if (parts.empty()) continue;

        const std::string cmd = parts[0];

        if (cmd == "NEW") {
            if (parts.size() != 7) {
                std::cerr << "NEW command requires 6 args. Type HELP.\n";
                continue;
            }

            uint64_t orderId = 0;
            try {
                orderId = std::stoull(parts[1]);
            } catch (...) {
                std::cerr << "Invalid orderId: " << parts[1] << "\n";
                continue;
            }

            std::string symbol = parts[2];
            std::string sideStr = parts[3];
            std::string typeStr = parts[4];

            double price = 0.0;
            uint32_t qty = 0;
            try {
                price = std::stod(parts[5]);
                qty = static_cast<uint32_t>(std::stoul(parts[6]));
            } catch (...) {
                std::cerr << "Invalid price/qty: " << parts[5] << " / " << parts[6] << "\n";
                continue;
            }

            // Validate side AFTER trimming whitespace
            Side side;
            if (sideStr == "BUY") side = Side::BUY;
            else if (sideStr == "SELL") side = Side::SELL;
            else {
                std::cerr << "Invalid side: " << sideStr << "\n";
                continue;
            }

            // Validate type AFTER trimming whitespace
            OrderType type;
            if (typeStr == "LIMIT") type = OrderType::LIMIT;
            else if (typeStr == "MARKET") type = OrderType::MARKET;
            else {
                std::cerr << "Invalid type: " << typeStr << "\n";
                continue;
            }

            // Build order
            Order o;
            o.orderId = orderId;
            o.symbol = symbol;
            o.side = side;
            o.type = type;

            // If MARKET, ensure price is normalized to 0.0 and rely on engine to not rest leftover
            if (o.type == OrderType::MARKET) {
                o.price = 0.0;
            } else {
                o.price = price;
            }

            o.quantity = qty;
            o.timestamp = now_nanos();

            // Optional debug (uncomment to inspect parsed tokens)
            // std::cout << "[DBG] Parsed NEW: id=" << o.orderId << " sym=" << o.symbol
            //           << " side=" << sideStr << " type=" << typeStr << " price=" << o.price
            //           << " qty=" << o.quantity << "\n";

            auto trades = book.addOrder(o);

            for (const auto& t : trades)
                printTradeJSON(t);

        } else if (cmd == "CANCEL") {
            if (parts.size() != 2) {
                std::cerr << "CANCEL requires orderId. Type HELP.\n";
                continue;
            }
            uint64_t orderId = 0;
            try {
                orderId = std::stoull(parts[1]);
            } catch (...) {
                std::cerr << "Invalid orderId: " << parts[1] << "\n";
                continue;
            }
            book.cancelOrder(orderId);

        } else {
            std::cerr << "Unknown command: " << cmd << ". Type HELP for usage.\n";
        }
    }

    std::cout << "Exiting.\n";
    return 0;
}
