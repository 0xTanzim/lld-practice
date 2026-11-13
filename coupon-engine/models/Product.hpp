#ifndef COUPON_PRODUCT_HPP
#define COUPON_PRODUCT_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../enums/ProductCategory.hpp"
#include "../money/Money.hpp"

class Product {
public:
    Product(std::string name, ProductCategory category, Money price)
        : name_(std::move(name)), category_(category), price_(std::move(price)) {
        if (name_.empty()) {
            throw std::invalid_argument("Product: name must not be empty");
        }
        if (price_.isNegative()) {
            throw std::invalid_argument("Product: price must not be negative");
        }
    }

    const std::string &name() const {
        return name_;
    }

    ProductCategory category() const {
        return category_;
    }

    const Money &price() const {
        return price_;
    }

    bool sameAs(const Product &other) const {
        return name_ == other.name_ && category_ == other.category_ && price_ == other.price_;
    }

private:
    std::string name_;
    ProductCategory category_;
    Money price_;
};

#endif // COUPON_PRODUCT_HPP
