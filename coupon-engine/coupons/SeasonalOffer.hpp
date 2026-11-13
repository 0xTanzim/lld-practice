#ifndef COUPON_SEASONAL_OFFER_HPP
#define COUPON_SEASONAL_OFFER_HPP

#include <memory>
#include <sstream>
#include <string>

#include "../coupons/Coupon.hpp"
#include "../enums/ProductCategory.hpp"
#include "../factories/DiscountStrategyFactory.hpp"
#include "../models/Cart.hpp"
#include "../strategies/DiscountStrategy.hpp"
#include "../utils/Format.hpp"

class SeasonalOffer : public Coupon {
public:
    SeasonalOffer(ProductCategory category, int basisPoints)
        : category_(category),
          basisPoints_(basisPoints),
          strategy_(DiscountStrategyFactory::percentage(basisPoints)) {}

    bool isApplicable(const Cart &cart) const override {
        return !cart.subtotalFor(category_).isZero();
    }

    Money discountFor(const Cart &cart) const override {
        return strategy_->discountFor(cart.subtotalFor(category_));
    }

    std::string name() const override {
        std::ostringstream out;
        out << "Seasonal offer — " << percentText(basisPoints_) << "% off " << toString(category_);
        return out.str();
    }

private:
    ProductCategory category_;
    int basisPoints_;
    std::unique_ptr<DiscountStrategy> strategy_;
};

#endif // COUPON_SEASONAL_OFFER_HPP
