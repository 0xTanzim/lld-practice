#ifndef COUPON_BANKING_COUPON_HPP
#define COUPON_BANKING_COUPON_HPP

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "../coupons/Coupon.hpp"
#include "../factories/DiscountStrategyFactory.hpp"
#include "../models/Cart.hpp"
#include "../strategies/DiscountStrategy.hpp"
#include "../utils/Format.hpp"

class BankingCoupon : public Coupon {
public:
    BankingCoupon(std::string bank, Money minSpend, int basisPoints, Money cap)
        : bank_(std::move(bank)),
          minSpend_(std::move(minSpend)),
          basisPoints_(basisPoints),
          cap_(std::move(cap)),
          strategy_(DiscountStrategyFactory::cappedPercentage(basisPoints, cap_)) {
        if (bank_.empty()) {
            throw std::invalid_argument("BankingCoupon: bank must not be empty");
        }
        if (minSpend_.isNegative()) {
            throw std::invalid_argument("BankingCoupon: minimum spend must not be negative");
        }
    }

    bool isApplicable(const Cart &cart) const override {
        return cart.customer().paymentBank == bank_ && cart.originalTotal() >= minSpend_;
    }

    Money discountFor(const Cart &cart) const override {
        return strategy_->discountFor(cart.currentTotal());
    }

    std::string name() const override {
        std::ostringstream out;
        out << bank_ << " — " << percentText(basisPoints_) << "% off up to " << cap_.format()
            << " (min spend " << minSpend_.format() << ")";
        return out.str();
    }

private:
    std::string bank_;
    Money minSpend_;
    int basisPoints_;
    Money cap_;
    std::unique_ptr<DiscountStrategy> strategy_;
};

#endif // COUPON_BANKING_COUPON_HPP
