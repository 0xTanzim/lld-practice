#ifndef QC_CATALOG_HPP
#define QC_CATALOG_HPP

#include <map>
#include <stdexcept>
#include <string>

#include "../models/Product.hpp"

// Product master data. Stores hold quantities, never their own copy of the
// product definition, so a price or name cannot drift per store.
class Catalog {
public:
    void add(Product product) {
        if (products_.count(product.sku()) != 0) {
            throw std::invalid_argument("Catalog: duplicate SKU " + std::to_string(product.sku()));
        }
        products_.emplace(product.sku(), std::move(product));
    }

    bool contains(int sku) const {
        return products_.count(sku) != 0;
    }

    const Product &require(int sku) const {
        const auto it = products_.find(sku);
        if (it == products_.end()) {
            throw std::out_of_range("Catalog: unknown SKU " + std::to_string(sku));
        }
        return it->second;
    }

private:
    std::map<int, Product> products_;
};

#endif // QC_CATALOG_HPP
