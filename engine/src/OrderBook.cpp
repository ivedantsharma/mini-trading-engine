#include "OrderBook.hpp"
#include <iostream>

void OrderBook::addOrder(const Order& order) {
    std::cout << "Order received: " 
              << order.orderId
              << " " << order.symbol 
              << " " << order.price 
              << " x " << order.quantity << "\n";
}

void OrderBook::printState() const {
    std::cout << "OrderBook state\n";
}
