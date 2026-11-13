#ifndef COUPON_CATALOG_HPP
#define COUPON_CATALOG_HPP

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../models/Product.hpp"

// Stand-in for the product service: prices can change here, but a cart keeps
// the price snapshot it captured when the item was added.
class Catalog {
public:
    void add(Product product) {
        if (find(product.name()) != nullptr) {
            throw std::invalid_argument("Catalog: product already exists: " + product.name());
        }
        products_.push_back(std::move(product));
    }

    void reprice(const std::string &name, Money price) {
        Product &product = mutableFind(name);
        product = Product(product.name(), product.category(), std::move(price));
    }

    const Product &require(const std::string &name) const {
        const Product *product = find(name);
        if (product == nullptr) {
            throw std::out_of_range("Catalog: unknown product: " + name);
        }
        return *product;
    }

    const Product *find(const std::string &name) const {
        for (const Product &product : products_) {
            if (product.name() == name) {
                return &product;
            }
        }
        return nullptr;
    }

private:
    Product &mutableFind(const std::string &name) {
        for (Product &product : products_) {
            if (product.name() == name) {
                return product;
            }
        }
        throw std::out_of_range("Catalog: unknown product: " + name);
    }

    std::vector<Product> products_;
};

#endif // COUPON_CATALOG_HPP
