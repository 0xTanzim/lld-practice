#ifndef QC_PRODUCT_HPP
#define QC_PRODUCT_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../money/Money.hpp"

class Product {
public:
    Product(int sku, std::string name, Money price)
        : sku_(sku), name_(std::move(name)), price_(price) {
        if (sku_ <= 0) {
            throw std::invalid_argument("Product: SKU must be positive");
        }
        if (name_.empty()) {
            throw std::invalid_argument("Product: name must not be empty");
        }
        if (price_.isNegative()) {
            throw std::invalid_argument("Product: price must not be negative");
        }
    }

    int sku() const {
        return sku_;
    }

    const std::string &name() const {
        return name_;
    }

    const Money &price() const {
        return price_;
    }

private:
    int sku_;
    std::string name_;
    Money price_;
};

#endif // QC_PRODUCT_HPP
