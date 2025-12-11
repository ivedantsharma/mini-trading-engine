#pragma once
#include <cstdint>
#include <string>
#include <map>

struct Position {
    int64_t qty = 0;        // positive = long, negative = short
    double avgPrice = 0.0;  // average entry price (meaningful only if qty != 0)
    double realizedPnl = 0.0;
};

// mark an order id as "user-submitted" (so subsequent trades referencing this order update portfolio)
void mark_user_order(uint64_t orderId);

// Called by engine code when a trade occurs — this will only update position if orderId was marked as user order.
// If isBuy == true => this order was the buyer side for this trade (so user bought).
void record_trade_for_order(uint64_t orderId, const std::string& symbol, bool isBuy, uint32_t qty, double price);

// Snapshot current positions (copy) for REST or broadcasting
std::map<std::string, Position> snapshot_positions();
