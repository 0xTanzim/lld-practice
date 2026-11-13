#ifndef COUPON_PRODUCT_CATEGORY_HPP
#define COUPON_PRODUCT_CATEGORY_HPP

enum class ProductCategory {
    CLOTHING,
    ELECTRONICS,
    GROCERY
};

inline const char *toString(ProductCategory category) {
    switch (category) {
    case ProductCategory::CLOTHING:
        return "Clothing";
    case ProductCategory::ELECTRONICS:
        return "Electronics";
    case ProductCategory::GROCERY:
        return "Grocery";
    }
    return "Unknown";
}

#endif // COUPON_PRODUCT_CATEGORY_HPP
