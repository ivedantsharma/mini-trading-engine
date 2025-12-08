#pragma once
#include <string>

class MarketDataServer {
public:
    static void start(unsigned short port);
    static void broadcast(const std::string& msg);
    static void stop();
};
