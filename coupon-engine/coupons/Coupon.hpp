#ifndef COUPON_COUPON_HPP
#define COUPON_COUPON_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../coupons/DiscountApplication.hpp"
#include "../models/Cart.hpp"

// Chain of Responsibility node with a fixed traversal contract: a coupon that
// applies hands control to the next link unless it opts out of combining.
// Non-combinable coupons stop the chain only when they actually reduced the
// total, so a rule that computes zero cannot silently starve later coupons.
class Coupon {
public:
    virtual ~Coupon() = default;

    void setNext(std::unique_ptr<Coupon> next) {
        next_ = std::move(next);
    }

    void process(Cart &cart, std::vector<DiscountApplication> &applied) const {
        bool appliedHere = false;
        if (isApplicable(cart)) {
            const Money requested = discountFor(cart);
            if (requested.isNegative()) {
                throw std::logic_error("Coupon: discount strategy returned a negative amount");
            }
            const Money amount = requested.min(cart.currentTotal());
            if (!amount.isZero()) {
                cart.applyDiscount(amount);
                applied.push_back(DiscountApplication{name(), amount});
                appliedHere = true;
            }
        }
        if (appliedHere && !isCombinable()) {
            return;
        }
        if (next_) {
            next_->process(cart, applied);
        }
    }

    void collectApplicable(const Cart &cart, std::vector<std::string> &names) const {
        if (isApplicable(cart)) {
            names.push_back(name());
        }
        if (next_) {
            next_->collectApplicable(cart, names);
        }
    }

    virtual bool isApplicable(const Cart &cart) const = 0;

    virtual Money discountFor(const Cart &cart) const = 0;

    virtual bool isCombinable() const {
        return true;
    }

    virtual std::string name() const = 0;

private:
    std::unique_ptr<Coupon> next_;
};

#endif // COUPON_COUPON_HPP
