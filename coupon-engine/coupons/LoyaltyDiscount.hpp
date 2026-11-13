#ifndef COUPON_LOYALTY_DISCOUNT_HPP
#define COUPON_LOYALTY_DISCOUNT_HPP

#include <memory>
#include <sstream>
#include <string>

#include "../coupons/Coupon.hpp"
#include "../factories/DiscountStrategyFactory.hpp"
#include "../models/Cart.hpp"
#include "../strategies/DiscountStrategy.hpp"
#include "../utils/Format.hpp"

class LoyaltyDiscount : public Coupon {
public:
    explicit LoyaltyDiscount(int basisPoints)
        : basisPoints_(basisPoints),
          strategy_(DiscountStrategyFactory::percentage(basisPoints)) {}

    bool isApplicable(const Cart &cart) const override {
        return cart.customer().loyaltyMember;
    }

    Money discountFor(const Cart &cart) const override {
        return strategy_->discountFor(cart.currentTotal());
    }

    std::string name() const override {
        std::ostringstream out;
        out << "Loyalty — " << percentText(basisPoints_) << "% off cart";
        return out.str();
    }

private:
    int basisPoints_;
    std::unique_ptr<DiscountStrategy> strategy_;
};

#endif // COUPON_LOYALTY_DISCOUNT_HPP
