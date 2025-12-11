#include "Positions.hpp"
#include <unordered_set>
#include <mutex>

static std::mutex g_mtx;
static std::unordered_set<uint64_t> g_userOrders;
static std::map<std::string, Position> g_positions;

void mark_user_order(uint64_t orderId) {
    std::lock_guard<std::mutex> g(g_mtx);
    g_userOrders.insert(orderId);
}

static void update_position_on_buy(Position &pos, uint32_t qty, double price) {
    // Buying:
    if (pos.qty >= 0) {
        // increase long
        double totalValue = pos.avgPrice * double(pos.qty) + price * double(qty);
        pos.qty += (int64_t)qty;
        pos.avgPrice = totalValue / double(pos.qty);
    } else {
        // covering short
        uint32_t cover = (uint32_t)std::min<int64_t>(qty, (uint64_t)(-pos.qty));
        // profit = (shortAvg - buyPrice) * coveredQty
        pos.realizedPnl += (pos.avgPrice - price) * double(cover);
        pos.qty += (int64_t)cover; // reduces magnitude of short
        if (qty > cover) {
            // remaining becomes new long
            uint32_t rem = qty - cover;
            pos.avgPrice = price;
            pos.qty += (int64_t)rem;
        }
        if (pos.qty == 0) pos.avgPrice = 0.0;
    }
}

static void update_position_on_sell(Position &pos, uint32_t qty, double price) {
    // Selling:
    if (pos.qty <= 0) {
        // increase short
        double totalValue = pos.avgPrice * double(-pos.qty) + price * double(qty);
        // store avgPrice as positive price for shorts (avg price of short entry)
        int64_t newQty = pos.qty - (int64_t)qty; // pos.qty is negative or zero
        if (pos.qty == 0) {
            // starting a short
            pos.avgPrice = price;
            pos.qty = - (int64_t) qty;
        } else {
            // existing short: compute weighted average price
            double prevNotional = pos.avgPrice * double(-pos.qty);
            double newNotional = prevNotional + price * double(qty);
            int64_t totalShortQty = -pos.qty + (int64_t)qty;
            pos.avgPrice = newNotional / double(totalShortQty);
            pos.qty = - totalShortQty;
        }
    } else {
        // selling into long => realize pnl
        uint32_t closeQty = (uint32_t)std::min<int64_t>(qty, pos.qty);
        pos.realizedPnl += (price - pos.avgPrice) * double(closeQty);
        pos.qty -= (int64_t)closeQty;
        if (qty > closeQty) {
            // extra sells create/extend short position
            uint32_t rem = qty - closeQty;
            pos.avgPrice = price;
            pos.qty -= (int64_t)rem;
        }
        if (pos.qty == 0) pos.avgPrice = 0.0;
    }
}

void record_trade_for_order(uint64_t orderId, const std::string& symbol, bool isBuy, uint32_t qty, double price) {
    std::lock_guard<std::mutex> g(g_mtx);
    if (g_userOrders.find(orderId) == g_userOrders.end()) return; // not our order, skip

    Position &pos = g_positions[symbol];

    if (isBuy) update_position_on_buy(pos, qty, price);
    else update_position_on_sell(pos, qty, price);
}

std::map<std::string, Position> snapshot_positions() {
    std::lock_guard<std::mutex> g(g_mtx);
    return g_positions; // copy
}
