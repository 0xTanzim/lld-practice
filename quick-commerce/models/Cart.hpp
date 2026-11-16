#ifndef QC_CART_HPP
#define QC_CART_HPP

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../models/Product.hpp"

// A cart line keeps the product by value, so the price is locked when the item
// is added. Repricing the catalog later cannot silently change a cart total.
struct CartLine {
    Product product;
    int quantity;

    CartLine(Product item, int count) : product(std::move(item)), quantity(count) {
        if (quantity <= 0) {
            throw std::invalid_argument("CartLine: quantity must be positive");
        }
    }

    Money lineTotal() const {
        return product.price() * quantity;
    }
};

class Cart {
public:
    void add(const Product &product, int quantity) {
        if (quantity <= 0) {
            throw std::invalid_argument("Cart: added quantity must be positive");
        }
        for (CartLine &line : lines_) {
            if (line.product.sku() == product.sku()) {
                if (line.product.price() != product.price()) {
                    throw std::invalid_argument("Cart: SKU " + std::to_string(product.sku())
                                                + " is already in the cart at a different price");
                }
                if (line.quantity > std::numeric_limits<int>::max() - quantity) {
                    throw std::overflow_error("Cart: quantity overflow for SKU "
                                              + std::to_string(product.sku()));
                }
                line.quantity += quantity;
                return;
            }
        }
        lines_.emplace_back(product, quantity);
    }

    const std::vector<CartLine> &lines() const {
        return lines_;
    }

    bool isEmpty() const {
        return lines_.empty();
    }

    std::size_t lineCount() const {
        return lines_.size();
    }

    int itemCount() const {
        int total = 0;
        for (const CartLine &line : lines_) {
            if (total > std::numeric_limits<int>::max() - line.quantity) {
                throw std::overflow_error("Cart: item count overflow");
            }
            total += line.quantity;
        }
        return total;
    }

    // Derived on every read: there is no cached total that can drift away from
    // the lines it is supposed to summarize.
    Money subtotal() const {
        Money total = Money::zero();
        for (const CartLine &line : lines_) {
            total = total + line.lineTotal();
        }
        return total;
    }

private:
    std::vector<CartLine> lines_;
};

#endif // QC_CART_HPP
