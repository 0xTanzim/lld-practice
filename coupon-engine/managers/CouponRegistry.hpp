#ifndef COUPON_REGISTRY_HPP
#define COUPON_REGISTRY_HPP

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../coupons/Coupon.hpp"
#include "../models/Cart.hpp"

// Owns the coupon chain for one checkout campaign. Kept per-service rather than
// process-wide so two carts cannot observe each other's campaign state.
class CouponRegistry {
public:
    void add(std::unique_ptr<Coupon> coupon) {
        if (!coupon) {
            throw std::invalid_argument("CouponRegistry: coupon must not be null");
        }
        Coupon *raw = coupon.get();
        if (!head_) {
            head_ = std::move(coupon);
            tail_ = raw;
        } else {
            tail_->setNext(std::move(coupon));
            tail_ = raw;
        }
        ++count_;
    }

    void applyAll(Cart &cart, std::vector<DiscountApplication> &applied) const {
        if (head_) {
            head_->process(cart, applied);
        }
    }

    std::vector<std::string> preview(const Cart &cart) const {
        std::vector<std::string> names;
        if (head_) {
            head_->collectApplicable(cart, names);
        }
        return names;
    }

    std::size_t size() const {
        return count_;
    }

    bool empty() const {
        return count_ == 0;
    }

private:
    std::unique_ptr<Coupon> head_;
    Coupon *tail_ = nullptr;
    std::size_t count_ = 0;
};

#endif // COUPON_REGISTRY_HPP
