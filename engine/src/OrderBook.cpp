#include "OrderBook.hpp"
#include <iostream>

OrderBook::OrderBook() {}

void OrderBook::addOrder(const Order& order) {

    if (order.type == OrderType::MARKET) {
        std::cout << "Market order received (matching coming in Phase 3)\n";
        return;
    }

    insertLimitOrder(order);
}

void OrderBook::insertLimitOrder(const Order& order) {
    if (order.side == Side::BUY) {
        bids[order.price].push_back(order);
    } else {
        asks[order.price].push_back(order);
    }

    // Store lookup for cancellation later
    orderIdToPrice[order.orderId] = order.price;

    std::cout << "Added LIMIT order: "
              << (order.side == Side::BUY ? "BUY " : "SELL ")
              << order.quantity << " @ " << order.price << "\n";
}

void OrderBook::cancelOrder(uint64_t orderId) {
    if (orderIdToPrice.find(orderId) == orderIdToPrice.end()) {
        std::cout << "Order not found\n";
        return;
    }

    double price = orderIdToPrice[orderId];

    // Determine which side it's on
    auto eraseFrom = [&](auto& book) {
        auto it = book.find(price);
        if (it == book.end()) return;

        auto& dequeAtPrice = it->second;

        for (auto iter = dequeAtPrice.begin(); iter != dequeAtPrice.end(); ++iter) {
            if (iter->orderId == orderId) {
                dequeAtPrice.erase(iter);
                break;
            }
        }

        // If price level empty, erase it
        if (dequeAtPrice.empty()) {
            book.erase(price);
        }
    };

    eraseFrom(bids);
    eraseFrom(asks);

    orderIdToPrice.erase(orderId);
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

