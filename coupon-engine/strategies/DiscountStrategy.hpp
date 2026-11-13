#ifndef COUPON_DISCOUNT_STRATEGY_HPP
#define COUPON_DISCOUNT_STRATEGY_HPP

#include <string>

#include "../money/Money.hpp"

class DiscountStrategy {
public:
    virtual ~DiscountStrategy() = default;

    virtual Money discountFor(const Money &base) const = 0;

    virtual std::string describe() const = 0;
};

#endif // COUPON_DISCOUNT_STRATEGY_HPP
