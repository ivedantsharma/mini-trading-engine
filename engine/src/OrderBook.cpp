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
    // market buy → match like limit buy with infinite price
    order.price = std::numeric_limits<double>::infinity();
    return matchLimitBuy(order);
}

std::vector<Trade> OrderBook::matchMarketSell(Order order) {
    // market sell → match like limit sell with price 0
    order.price = 0.0;
    return matchLimitSell(order);
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
    auto it_lookup = orderIdToPrice.find(orderId);
    if (it_lookup == orderIdToPrice.end()) {
        std::cout << "Order not found\n";
        return;
    }

    double price = it_lookup->second;

    // Determine which side it's on by trying both books.
    auto eraseFrom = [&](auto& book) {
        auto it = book.find(price);
        if (it == book.end()) return false;

        auto& dequeAtPrice = it->second;

        for (auto iter = dequeAtPrice.begin(); iter != dequeAtPrice.end(); ++iter) {
            if (iter->orderId == orderId) {
                dequeAtPrice.erase(iter);
                // If price level empty, erase it
                if (dequeAtPrice.empty()) {
                    book.erase(it);
                }
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
        // Stale mapping or already filled
        std::cout << "Order not found in book (maybe already filled) - cleaning lookup\n";
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

    // While we have sell orders and incoming order has quantity
    while (!asks.empty() && order.quantity > 0) {
        auto bestAskIt = asks.begin();
        double bestAskPrice = bestAskIt->first;

        // If buy price < best ask → can't match
        if (order.price < bestAskPrice)
            break;

        auto &sellQueue = bestAskIt->second;

        // Reference to the front (resting) sell order
        Order &sellOrder = sellQueue.front();

        uint32_t tradedQty = std::min(order.quantity, sellOrder.quantity);

        // Create trade - trade price is resting order's price (best ask)
        trades.push_back(Trade{
            nextTradeId++,
            order.orderId,          // buy ID
            sellOrder.orderId,      // sell ID
            bestAskPrice,           // trade happens at ask price
            tradedQty,
            order.timestamp         // simple timestamp
        });

        // Decrease quantities
        order.quantity -= tradedQty;
        sellOrder.quantity -= tradedQty;

        // If sell order fully filled, remove it AND its lookup entry
        if (sellOrder.quantity == 0) {
            uint64_t filledOrderId = sellOrder.orderId;
            // Erase mapping for this filled resting order
            auto it = orderIdToPrice.find(filledOrderId);
            if (it != orderIdToPrice.end()) orderIdToPrice.erase(it);

            sellQueue.pop_front();
        }

        // If price level empty, erase it
        if (sellQueue.empty())
            asks.erase(bestAskIt);
    }

    // If remaining qty → insert into bids
    if (order.quantity > 0) {
        insertLimitOrder(order);
    }

    return trades;
}

std::vector<Trade> OrderBook::matchLimitSell(Order order) {
    std::vector<Trade> trades;

    while (!bids.empty() && order.quantity > 0) {
        auto bestBidIt = bids.begin();
        double bestBidPrice = bestBidIt->first;

        // If sell price > best bid → no match
        if (order.price > bestBidPrice)
            break;

        auto &buyQueue = bestBidIt->second;

        // Resting buy order
        Order &buyOrder = buyQueue.front();

        uint32_t tradedQty = std::min(order.quantity, buyOrder.quantity);

        // Create trade - trade price is resting order's price (best bid)
        trades.push_back(Trade{
            nextTradeId++,
            buyOrder.orderId,      // buy ID
            order.orderId,         // sell ID
            bestBidPrice,
            tradedQty,
            order.timestamp
        });

        order.quantity -= tradedQty;
        buyOrder.quantity -= tradedQty;

        // If buy order fully filled, remove it AND its lookup entry
        if (buyOrder.quantity == 0) {
            uint64_t filledOrderId = buyOrder.orderId;
            auto it = orderIdToPrice.find(filledOrderId);
            if (it != orderIdToPrice.end()) orderIdToPrice.erase(it);

            buyQueue.pop_front();
        }

        if (buyQueue.empty())
            bids.erase(bestBidIt);
    }

    if (order.quantity > 0) {
        insertLimitOrder(order);
    }

    return trades;
}
