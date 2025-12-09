#include "OrderBook.hpp"
#include <iostream>
#include <limits>
#include <algorithm>

OrderBook::OrderBook() {}

std::vector<Trade> OrderBook::addOrder(const Order& order) {
    if (order.type == OrderType::MARKET) {
        if (order.side == Side::BUY)
            return matchMarketBuy(order);
        else
            return matchMarketSell(order);
    }

    if (order.side == Side::BUY)
        return matchLimitBuy(order);
    else
        return matchLimitSell(order);
}

std::vector<Trade> OrderBook::matchMarketBuy(Order order) {
    order.price = std::numeric_limits<double>::infinity(); // can match all asks
    auto trades = matchLimitBuy(order);
    // Market orders NEVER rest
    return trades;
}

std::vector<Trade> OrderBook::matchMarketSell(Order order) {
    order.price = 0.0; // can match all bids
    auto trades = matchLimitSell(order);
    // Market orders NEVER rest
    return trades;
}

void OrderBook::insertLimitOrder(const Order& order) {
    if (order.side == Side::BUY) {
        bids[order.price].push_back(order);
    } else {
        asks[order.price].push_back(order);
    }

    orderIdToPrice[order.orderId] = order.price;

    std::cout << "Added LIMIT order: "
              << (order.side == Side::BUY ? "BUY " : "SELL ")
              << order.quantity << " @ " << order.price << "\n";
}

void OrderBook::cancelOrder(uint64_t orderId) {
    auto it_lookup = orderIdToPrice.find(orderId);
    if (it_lookup == orderIdToPrice.end()) {
        std::cout << "Order not found\n";
        return;
    }

    double price = it_lookup->second;

    auto eraseFrom = [&](auto& book) {
        auto it = book.find(price);
        if (it == book.end()) return false;

        auto& dq = it->second;

        for (auto iter = dq.begin(); iter != dq.end(); ++iter) {
            if (iter->orderId == orderId) {
                dq.erase(iter);
                if (dq.empty()) book.erase(it);
                return true;
            }
        }
        return false;
    };

    bool erased = eraseFrom(bids) || eraseFrom(asks);

    if (erased) {
        orderIdToPrice.erase(it_lookup);
        std::cout << "Cancelled order " << orderId << "\n";
    } else {
        std::cout << "Order not found in book (maybe filled) – cleaning lookup\n";
        orderIdToPrice.erase(it_lookup);
    }
}

void OrderBook::printTopLevels() const {
    std::cout << "Top of Book:\n";

    if (!bids.empty()) {
        auto bestBid = bids.begin();
        std::cout << "Best Bid: " << bestBid->first
                  << " (" << bestBid->second.size() << " orders)\n";
    } else {
        std::cout << "Best Bid: None\n";
    }

    if (!asks.empty()) {
        auto bestAsk = asks.begin();
        std::cout << "Best Ask: " << bestAsk->first
                  << " (" << bestAsk->second.size() << " orders)\n";
    } else {
        std::cout << "Best Ask: None\n";
    }
}

std::vector<Trade> OrderBook::matchLimitBuy(Order order) {
    std::vector<Trade> trades;

    while (!asks.empty() && order.quantity > 0) {
        auto bestAskIt = asks.begin();
        double bestAskPrice = bestAskIt->first;

        if (order.price < bestAskPrice)
            break;

        auto& sellQueue = bestAskIt->second;
        Order& sellOrder = sellQueue.front();

        uint32_t tradedQty = std::min(order.quantity, sellOrder.quantity);

        std::cout << "[DEBUG] MATCH BUY: bestAskPrice=" << bestAskPrice
                  << " order.price=" << order.price
                  << " tradedQty=" << tradedQty << std::endl;

        trades.push_back(Trade{
            nextTradeId++,
            order.orderId,
            sellOrder.orderId,
            bestAskPrice,
            tradedQty,
            order.timestamp
        });

        order.quantity -= tradedQty;
        sellOrder.quantity -= tradedQty;

        if (sellOrder.quantity == 0) {
            auto it = orderIdToPrice.find(sellOrder.orderId);
            if (it != orderIdToPrice.end()) orderIdToPrice.erase(it);
            sellQueue.pop_front();
        }

        if (sellQueue.empty())
            asks.erase(bestAskIt);
    }

    // FIX: leftover qty must NOT be inserted if MARKET order
    if (order.quantity > 0 && order.type == OrderType::LIMIT) {
        insertLimitOrder(order);
    }

    return trades;
}

std::vector<Trade> OrderBook::matchLimitSell(Order order) {
    std::vector<Trade> trades;

    while (!bids.empty() && order.quantity > 0) {
        auto bestBidIt = bids.begin();
        double bestBidPrice = bestBidIt->first;

        if (order.price > bestBidPrice)
            break;

        auto& buyQueue = bestBidIt->second;
        Order& buyOrder = buyQueue.front();

        uint32_t tradedQty = std::min(order.quantity, buyOrder.quantity);

        std::cout << "[DEBUG] MATCH SELL: bestBidPrice=" << bestBidPrice
                  << " order.price=" << order.price
                  << " tradedQty=" << tradedQty << std::endl;

        trades.push_back(Trade{
            nextTradeId++,
            buyOrder.orderId,
            order.orderId,
            bestBidPrice,
            tradedQty,
            order.timestamp
        });

        order.quantity -= tradedQty;
        buyOrder.quantity -= tradedQty;

        if (buyOrder.quantity == 0) {
            auto it = orderIdToPrice.find(buyOrder.orderId);
            if (it != orderIdToPrice.end()) orderIdToPrice.erase(it);
            buyQueue.pop_front();
        }

        if (buyQueue.empty())
            bids.erase(bestBidIt);
    }

    // FIX here also
    if (order.quantity > 0 && order.type == OrderType::LIMIT) {
        insertLimitOrder(order);
    }

    return trades;
}

std::vector<DepthLevel> OrderBook::getDepth(bool isBid, int levels) const {
    std::vector<DepthLevel> out;
    out.reserve(levels);

    if (isBid) {
        for (auto it = bids.begin(); it != bids.end() && levels > 0; ++it) {
            uint32_t sum = 0;
            for (auto &o : it->second) sum += o.quantity;
            out.push_back({it->first, sum});
            levels--;
        }
    } else {
        for (auto it = asks.begin(); it != asks.end() && levels > 0; ++it) {
            uint32_t sum = 0;
            for (auto &o : it->second) sum += o.quantity;
            out.push_back({it->first, sum});
            levels--;
        }
    }

    return out;
}

