#ifndef COUPON_PERCENTAGE_DISCOUNT_HPP
#define COUPON_PERCENTAGE_DISCOUNT_HPP

#include <limits>
#include <stdexcept>
#include <string>

#include "../money/Money.hpp"
#include "../strategies/DiscountStrategy.hpp"
#include "../utils/Format.hpp"

class PercentageDiscount : public DiscountStrategy {
public:
    explicit PercentageDiscount(int basisPoints) : basisPoints_(basisPoints) {
        if (basisPoints < 0 || basisPoints > MAX_BASIS_POINTS) {
            throw std::invalid_argument("PercentageDiscount: basis points must be within 0..10000");
        }
    }

    Money discountFor(const Money &base) const override {
        if (base.isNegative()) {
            throw std::invalid_argument("PercentageDiscount: base amount must not be negative");
        }
        if (base.minor() > 0 && basisPoints_ > 0
            && base.minor() > std::numeric_limits<Money::Minor>::max() / basisPoints_) {
            throw std::overflow_error("PercentageDiscount: amount overflow");
        }
        const Money::Minor discounted = base.minor() * basisPoints_ / MAX_BASIS_POINTS;
        return Money::fromMinor(discounted, base.currency());
    }

    std::string describe() const override {
        return percentText(basisPoints_) + "% off";
    }

private:
    static constexpr int MAX_BASIS_POINTS = 10000;
    int basisPoints_;
};

#endif // COUPON_PERCENTAGE_DISCOUNT_HPP
