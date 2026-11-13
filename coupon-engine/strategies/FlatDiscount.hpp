#ifndef COUPON_FLAT_DISCOUNT_HPP
#define COUPON_FLAT_DISCOUNT_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../money/Money.hpp"
#include "../strategies/DiscountStrategy.hpp"

class FlatDiscount : public DiscountStrategy {
public:
    explicit FlatDiscount(Money amount) : amount_(std::move(amount)) {
        if (amount_.isNegative()) {
            throw std::invalid_argument("FlatDiscount: amount must not be negative");
        }
    }

    Money discountFor(const Money &base) const override {
        return amount_.min(base).clampToZero();
    }

    std::string describe() const override {
        return amount_.format() + " off";
    }

private:
    Money amount_;
};

#endif // COUPON_FLAT_DISCOUNT_HPP
