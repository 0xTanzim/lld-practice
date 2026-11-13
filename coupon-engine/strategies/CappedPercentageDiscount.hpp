#ifndef COUPON_CAPPED_PERCENTAGE_DISCOUNT_HPP
#define COUPON_CAPPED_PERCENTAGE_DISCOUNT_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../money/Money.hpp"
#include "../strategies/DiscountStrategy.hpp"
#include "../strategies/PercentageDiscount.hpp"

class CappedPercentageDiscount : public DiscountStrategy {
public:
    CappedPercentageDiscount(int basisPoints, Money cap)
        : percentage_(basisPoints), cap_(std::move(cap)) {
        if (cap_.isNegative()) {
            throw std::invalid_argument("CappedPercentageDiscount: cap must not be negative");
        }
    }

    Money discountFor(const Money &base) const override {
        return percentage_.discountFor(base).min(cap_).clampToZero();
    }

    std::string describe() const override {
        return percentage_.describe() + " up to " + cap_.format();
    }

private:
    PercentageDiscount percentage_;
    Money cap_;
};

#endif // COUPON_CAPPED_PERCENTAGE_DISCOUNT_HPP
