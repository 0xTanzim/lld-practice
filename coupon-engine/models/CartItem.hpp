#ifndef COUPON_CART_ITEM_HPP
#define COUPON_CART_ITEM_HPP

#include <limits>
#include <stdexcept>
#include <utility>

#include "../models/Product.hpp"

// Stores the product by value: the price is locked when the item enters the
// cart, so later catalog edits cannot silently rewrite an existing cart total.
class CartItem {
public:
    CartItem(Product product, int quantity) : product_(std::move(product)), quantity_(quantity) {
        if (quantity <= 0) {
            throw std::invalid_argument("CartItem: quantity must be positive");
        }
    }

    const Product &product() const {
        return product_;
    }

    int quantity() const {
        return quantity_;
    }

    Money unitPrice() const {
        return product_.price();
    }

    Money subtotal() const {
        return product_.price() * quantity_;
    }

    bool matches(const Product &candidate) const {
        return product_.sameAs(candidate);
    }

    void increaseQuantity(int extra) {
        if (extra <= 0) {
            throw std::invalid_argument("CartItem: added quantity must be positive");
        }
        if (quantity_ > std::numeric_limits<int>::max() - extra) {
            throw std::overflow_error("CartItem: quantity overflow");
        }
        quantity_ += extra;
    }

private:
    Product product_;
    int quantity_;
};

#endif // COUPON_CART_ITEM_HPP
