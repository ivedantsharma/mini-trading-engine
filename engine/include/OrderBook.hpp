#pragma once
#include "Order.hpp"
#include <vector>

class OrderBook {
public:
    void addOrder(const Order& order);
    void printState() const;
};
