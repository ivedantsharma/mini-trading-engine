#pragma once

#include <string>
#include <memory>

class MarketDataServer {
public:
    // Start server on given port (non-blocking). Call once in main.
    static void start(unsigned short port);

    // Stop server and join thread (optional)
    static void stop();

    // Broadcast a JSON line (thread-safe). If no clients, still prints to stderr.
    static void broadcast(const std::string& json);

private:
    MarketDataServer() = default;
    MarketDataServer(const MarketDataServer&) = delete;
    MarketDataServer& operator=(const MarketDataServer&) = delete;
};
