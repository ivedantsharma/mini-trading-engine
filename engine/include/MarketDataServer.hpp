#pragma once
#include <string>

namespace MarketDataServerAPI {
// Start server on port (non-blocking)
void start(unsigned short port);

// Broadcast JSON/text line to all connected WS clients (thread-safe)
void broadcast(const std::string& msg);

// Try to pop one client-sent message (non-blocking).
// Returns true and sets out if a message was available.
bool try_pop_client_message(std::string &out);

// Stop server (join thread)
void stop();
} // namespace MarketDataServerAPI
