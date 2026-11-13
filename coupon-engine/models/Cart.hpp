#ifndef COUPON_CART_HPP
#define COUPON_CART_HPP

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../enums/Currency.hpp"
#include "../enums/ProductCategory.hpp"
#include "../models/CartItem.hpp"
#include "../models/Customer.hpp"
#include "../models/Product.hpp"

// Totals are derived from the items on every read; there is no cached total
// that could drift away from the line items it is supposed to summarize.
class Cart {
public:
    Cart(Currency currency, Customer customer)
        : currency_(currency), customer_(std::move(customer)), discountTotal_(Money::zero(currency)) {}

    void add(const Product &product, int quantity = 1) {
        if (product.price().currency() != currency_) {
            throw std::invalid_argument(std::string("Cart: product priced in ")
                                        + toString(product.price().currency())
                                        + " cannot join a " + toString(currency_) + " cart");
        }
        for (CartItem &item : items_) {
            if (item.matches(product)) {
                item.increaseQuantity(quantity);
                return;
            }
        }
        items_.emplace_back(product, quantity);
    }

    Currency currency() const {
        return currency_;
    }

    const Customer &customer() const {
        return customer_;
    }

    const std::vector<CartItem> &items() const {
        return items_;
    }

    bool isEmpty() const {
        return items_.empty();
    }

    Money originalTotal() const {
        Money total = Money::zero(currency_);
        for (const CartItem &item : items_) {
            total = total + item.subtotal();
        }
        return total;
    }

    Money discountTotal() const {
        return discountTotal_;
    }

    Money currentTotal() const {
        return originalTotal() - discountTotal_;
    }

    void applyDiscount(const Money &discount) {
        if (discount.isNegative()) {
            throw std::invalid_argument("Cart: discount must not be negative");
        }
        if (discount.currency() != currency_) {
            throw std::invalid_argument("Cart: discount currency does not match cart currency");
        }
        discountTotal_ = discountTotal_ + discount;
        const Money original = originalTotal();
        if (original < discountTotal_) {
            discountTotal_ = original;
        }
    }

    Money subtotalFor(ProductCategory category) const {
        Money subtotal = Money::zero(currency_);
        for (const CartItem &item : items_) {
            if (item.product().category() == category) {
                subtotal = subtotal + item.subtotal();
            }
        }
        return subtotal;
    }

private:
    Currency currency_;
    Customer customer_;
    std::vector<CartItem> items_;
    Money discountTotal_;
};

#endif // COUPON_CART_HPP
