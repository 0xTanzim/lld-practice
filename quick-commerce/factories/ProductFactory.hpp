#ifndef QC_PRODUCT_FACTORY_HPP
#define QC_PRODUCT_FACTORY_HPP

#include <initializer_list>
#include <stdexcept>
#include <string>

#include "../models/Catalog.hpp"
#include "../models/Product.hpp"
#include "../money/Money.hpp"

// Factory for the product master. Every SKU the shop sells is created here, so
// a name and a price exist in exactly one place.
//
// The original factory answered an unknown code with a made-up product called
// "Item<sku>" at a default price, which turned a typo into stock. Here an
// unknown code throws.
class ProductFactory {
public:
    static constexpr int APPLE = 101;
    static constexpr int BANANA = 102;
    static constexpr int CHOCOLATE = 103;
    static constexpr int TSHIRT = 201;
    static constexpr int JEANS = 202;

    static Product create(int sku) {
        switch (sku) {
        case APPLE:
            return Product(APPLE, "Apple", Money::taka(20));
        case BANANA:
            return Product(BANANA, "Banana", Money::taka(10));
        case CHOCOLATE:
            return Product(CHOCOLATE, "Chocolate", Money::taka(50));
        case TSHIRT:
            return Product(TSHIRT, "T-Shirt", Money::taka(500));
        case JEANS:
            return Product(JEANS, "Jeans", Money::taka(1000));
        default:
            throw std::invalid_argument("ProductFactory: unknown SKU " + std::to_string(sku));
        }
    }

    static Catalog buildCatalog() {
        Catalog catalog;
        for (int sku : {APPLE, BANANA, CHOCOLATE, TSHIRT, JEANS}) {
            catalog.add(create(sku));
        }
        return catalog;
    }
};

#endif // QC_PRODUCT_FACTORY_HPP
