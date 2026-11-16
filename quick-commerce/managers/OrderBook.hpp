#ifndef QC_ORDER_BOOK_HPP
#define QC_ORDER_BOOK_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../models/Order.hpp"
#include "../money/Money.hpp"

// The record of accepted orders. Ids are assigned here and nowhere else, so a
// rejected checkout cannot consume one - the original used a static counter
// that advanced even when the order never existed.
class OrderBook {
public:
    int peekNextId() const {
        return static_cast<int>(orders_.size()) + 1;
    }

    void add(Order order) {
        if (order.id() != peekNextId()) {
            throw std::logic_error("OrderBook: order id " + std::to_string(order.id())
                                   + " does not follow the book sequence (expected "
                                   + std::to_string(peekNextId()) + ")");
        }
        orders_.push_back(std::move(order));
    }

    const std::vector<Order> &orders() const {
        return orders_;
    }

    std::size_t size() const {
        return orders_.size();
    }

    Money capturedRevenue() const {
        Money total = Money::zero();
        for (const Order &order : orders_) {
            if (order.isPaid()) {
                total = total + order.summary().total();
            }
        }
        return total;
    }

    Money pendingRevenue() const {
        Money total = Money::zero();
        for (const Order &order : orders_) {
            if (!order.isPaid()) {
                total = total + order.summary().total();
            }
        }
        return total;
    }

private:
    std::vector<Order> orders_;
};

#endif // QC_ORDER_BOOK_HPP
