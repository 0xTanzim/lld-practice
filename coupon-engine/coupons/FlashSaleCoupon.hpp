#ifndef COUPON_FLASH_SALE_COUPON_HPP
#define COUPON_FLASH_SALE_COUPON_HPP

#include <memory>
#include <sstream>
#include <string>

#include "../coupons/Coupon.hpp"
#include "../factories/DiscountStrategyFactory.hpp"
#include "../models/Cart.hpp"
#include "../strategies/DiscountStrategy.hpp"
#include "../utils/Format.hpp"

// Non-combinable campaign: once it reduces the total, no later coupon runs.
class FlashSaleCoupon : public Coupon {
public:
    explicit FlashSaleCoupon(int basisPoints)
        : basisPoints_(basisPoints),
          strategy_(DiscountStrategyFactory::percentage(basisPoints)) {}

    bool isApplicable(const Cart &cart) const override {
        return !cart.isEmpty();
    }

    Money discountFor(const Cart &cart) const override {
        return strategy_->discountFor(cart.currentTotal());
    }

    bool isCombinable() const override {
        return false;
    }

    std::string name() const override {
        std::ostringstream out;
        out << "Flash sale — " << percentText(basisPoints_) << "% off cart (exclusive)";
        return out.str();
    }

private:
    int basisPoints_;
    std::unique_ptr<DiscountStrategy> strategy_;
};

#endif // COUPON_FLASH_SALE_COUPON_HPP
