#ifndef COUPON_CHECKOUT_SERVICE_HPP
#define COUPON_CHECKOUT_SERVICE_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../checkout/CheckoutReceipt.hpp"
#include "../coupons/Coupon.hpp"
#include "../managers/CouponRegistry.hpp"
#include "../models/Cart.hpp"

// Facade over campaign registration and checkout. Checkout runs on a copy of
// the cart, so the caller's cart is never mutated and the operation is safe to
// repeat with an identical result.
class CheckoutService {
public:
    void addCoupon(std::unique_ptr<Coupon> coupon) {
        registry_.add(std::move(coupon));
    }

    std::vector<std::string> preview(const Cart &cart) const {
        return registry_.preview(cart);
    }

    CheckoutReceipt checkout(const Cart &cart) const {
        if (cart.isEmpty()) {
            throw std::invalid_argument("CheckoutService: cannot check out an empty cart");
        }
        if (!cart.discountTotal().isZero()) {
            throw std::invalid_argument("CheckoutService: cart already has discounts applied");
        }

        Cart working = cart;
        std::vector<DiscountApplication> applied;
        registry_.applyAll(working, applied);

        return CheckoutReceipt(working.customer().name, working.currency(),
                               working.originalTotal(), std::move(applied),
                               working.currentTotal());
    }

    std::size_t couponCount() const {
        return registry_.size();
    }

private:
    CouponRegistry registry_;
};

#endif // COUPON_CHECKOUT_SERVICE_HPP
