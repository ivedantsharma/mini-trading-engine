#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <unistd.h>
#include "DBLogger.hpp"
#include "OrderBookManager.hpp"
#include "MarketDataServer.hpp"

static DBLogger DB;

// helper to get monotonic timestamp in nanoseconds
static uint64_t now_nanos() {
    using namespace std::chrono;
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void printTradeJSON(const Trade& t) {
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
              << "  CANCEL,<orderId>\n"
              << "  SNAP or SNAP,<SYMBOL>\n"
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
    // Initialize database logger
    DB.init("trading.db");
    
    // Start WS market-data server
    unsigned short md_port = 9002;
    MarketDataServerAPI::start(md_port);

    OrderBookManager mgr;
    std::string line;

    std::cout << "Mini Trading Engine CLI (type HELP for usage)\n";

    auto process_line = [&](const std::string &l_in) {
        std::string l = l_in;
        if (l.empty()) return;
        // ignore comments
        if (l[0] == '#') return;
        auto pos = l.find('#');
        if (pos != std::string::npos) l = l.substr(0, pos);
        l = trim(l);
        if (l.empty()) return;
        if (l == "QUIT" || l == "EXIT") {
            // indicate quit by throwing a signal via exception or return code
            throw std::runtime_error("QUIT");
        }
        if (l == "HELP") { printUsage(); return; }
        if (l == "SNAP") { mgr.printTopLevels(); return; }

        // SNAP for specific symbol
        if (l.rfind("SNAP,", 0) == 0) {
            std::string symbol = trim(l.substr(5));
            mgr.printTopLevels(symbol);
            return;
        }

        // CSV parse
        std::stringstream ss(l);
        std::string token;
        std::vector<std::string> parts;
        while (std::getline(ss, token, ',')) {
            parts.push_back(trim(token));
        }
        if (parts.empty()) return;

        const std::string cmd = parts[0];
        if (cmd == "NEW") {
            if (parts.size() != 7) {
                std::cerr << "NEW command requires 6 args. Type HELP.\n";
                return;
            }
            uint64_t orderId = 0;
            try { orderId = std::stoull(parts[1]); } catch(...) { std::cerr << "Invalid orderId\n"; return; }
            std::string symbol = parts[2];
            std::string sideStr = parts[3];
            std::string typeStr = parts[4];
            double price = 0.0;
            uint32_t qty = 0;
            try { price = std::stod(parts[5]); qty = static_cast<uint32_t>(std::stoul(parts[6])); } catch(...) { std::cerr << "Invalid price/qty\n"; return; }

            Side side;
            if (sideStr == "BUY") side = Side::BUY;
            else if (sideStr == "SELL") side = Side::SELL;
            else { std::cerr << "Invalid side\n"; return; }

            OrderType type;
            if (typeStr == "LIMIT") type = OrderType::LIMIT;
            else if (typeStr == "MARKET") type = OrderType::MARKET;
            else { std::cerr << "Invalid type\n"; return; }

            Order o;
            o.orderId = orderId;
            o.symbol = symbol;
            o.side = side;
            o.type = type;
            o.price = (type == OrderType::MARKET ? 0.0 : price);
            o.quantity = qty;
            o.timestamp = now_nanos();

            auto trades = mgr.addOrder(symbol, o);
            DB.logOrder(o);
            for (const auto &t : trades) {
                DB.logTrade(t, symbol);
            }
            for (const auto &t : trades) printTradeJSON(t);

        } else if (cmd == "CANCEL") {
            if (parts.size() != 3 && parts.size() != 2) {
                std::cerr << "CANCEL requires orderId and optional symbol (CANCEL,<symbol>,<orderId> or CANCEL,<orderId>)\n";
                return;
            }
            if (parts.size() == 3) {
                std::string symbol = parts[1];
                uint64_t orderId = std::stoull(parts[2]);
                mgr.cancelOrder(symbol, orderId);
            } else {
                uint64_t orderId = std::stoull(parts[1]);
                // if symbol missing, attempt cancel across books by probing or just print message
                // For simplicity, try all books
                // (OrderBookManager::cancelOrder currently expects symbol; to cancel globally we'd add code)
                // Here we just print message: user should pass symbol
                std::cerr << "Please provide symbol for cancel: CANCEL,<symbol>,<orderId>\n";
            }
        } else {
            std::cerr << "Unknown command: " << cmd << "\n";
        }
    };

    try {
        // main loop: process queued client messages first, then stdin input
        while (true) {
            // Handle client-sent messages (WS) — non-blocking: process all available
            std::string client_msg;
            while (MarketDataServerAPI::try_pop_client_message(client_msg)) {
                try {
                    process_line(client_msg);
                } catch (const std::runtime_error &e) {
                    if (std::string(e.what()) == "QUIT") throw;
                } catch(...) {}
            }

            // Now read a line from stdin (blocking)
            if (!isatty(fileno(stdin))) {
                // if input redirected (script), just read as usual
                if (!std::getline(std::cin, line)) break;
                try { process_line(line); } catch(const std::runtime_error &e) { if (std::string(e.what()) == "QUIT") break; }
            } else {
                // interactive: show prompt and read; also we can do a short sleep to avoid busy-loop
                std::cout << "> " << std::flush;
                if (!std::getline(std::cin, line)) break;
                try { process_line(line); } catch(const std::runtime_error &e) { if (std::string(e.what()) == "QUIT") break; }
            }
        }
    } catch(...) {
        // fallthrough to cleanup
    }

    MarketDataServerAPI::stop();
    std::cout << "Exiting.\n";
    return 0;
}
