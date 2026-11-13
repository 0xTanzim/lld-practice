#ifndef COUPON_DISCOUNT_STRATEGY_FACTORY_HPP
#define COUPON_DISCOUNT_STRATEGY_FACTORY_HPP

#include <memory>

#include "../money/Money.hpp"
#include "../strategies/CappedPercentageDiscount.hpp"
#include "../strategies/DiscountStrategy.hpp"
#include "../strategies/FlatDiscount.hpp"
#include "../strategies/PercentageDiscount.hpp"

// Central place for strategy construction and parameter validation, so coupon
// rules cannot be assembled with an invalid percentage or negative cap.
class DiscountStrategyFactory {
public:
    DiscountStrategyFactory() = delete;

    static std::unique_ptr<DiscountStrategy> flat(Money amount) {
        return std::make_unique<FlatDiscount>(std::move(amount));
    }

    static std::unique_ptr<DiscountStrategy> percentage(int basisPoints) {
        return std::make_unique<PercentageDiscount>(basisPoints);
    }

    static std::unique_ptr<DiscountStrategy> cappedPercentage(int basisPoints, Money cap) {
        return std::make_unique<CappedPercentageDiscount>(basisPoints, std::move(cap));
    }
};

#endif // COUPON_DISCOUNT_STRATEGY_FACTORY_HPP
