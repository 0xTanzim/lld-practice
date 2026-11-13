#ifndef COUPON_BULK_PURCHASE_DISCOUNT_HPP
#define COUPON_BULK_PURCHASE_DISCOUNT_HPP

#include <memory>
#include <string>
#include <utility>

#include "../coupons/Coupon.hpp"
#include "../factories/DiscountStrategyFactory.hpp"
#include "../models/Cart.hpp"
#include "../strategies/DiscountStrategy.hpp"

class BulkPurchaseDiscount : public Coupon {
public:
    BulkPurchaseDiscount(Money threshold, Money flatOff)
        : threshold_(std::move(threshold)),
          flatOff_(std::move(flatOff)),
          strategy_(DiscountStrategyFactory::flat(flatOff_)) {}

    bool isApplicable(const Cart &cart) const override {
        return cart.originalTotal() >= threshold_;
    }

    Money discountFor(const Cart &cart) const override {
        return strategy_->discountFor(cart.currentTotal());
    }

    std::string name() const override {
        return "Bulk purchase — " + flatOff_.format() + " off over " + threshold_.format();
    }

private:
    Money threshold_;
    Money flatOff_;
    std::unique_ptr<DiscountStrategy> strategy_;
};

#endif // COUPON_BULK_PURCHASE_DISCOUNT_HPP
