#include "../include/OrderBook.hpp"

int main() {
    OrderBook book;

    Order o1{1, "AAPL", Side::BUY, OrderType::LIMIT, 100.5, 10, 1};
    Order o2{2, "AAPL", Side::SELL, OrderType::LIMIT, 101.0, 5, 2};

    book.addOrder(o1);
    book.addOrder(o2);

    book.printTopLevels();

    return 0;
}
