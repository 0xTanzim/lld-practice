#ifndef QC_COUPON_HPP
#define QC_COUPON_HPP

#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "../errors/CheckoutError.hpp"
#include "../money/Money.hpp"

// A single-code promo. Deliberately small: one percentage off the item
// subtotal with a minimum spend. It is applied by the checkout service, after
// allocation, so a rejected code costs nothing but a released reservation.
class Coupon {
public:
    Coupon(std::string code, int basisPoints, Money minSpend)
        : code_(std::move(code)), basisPoints_(basisPoints), minSpend_(minSpend) {
        if (code_.empty()) {
            throw std::invalid_argument("Coupon: code must not be empty");
        }
        if (basisPoints_ < 0 || basisPoints_ > MAX_BASIS_POINTS) {
            throw std::invalid_argument("Coupon: basis points must be within 0..10000");
        }
        if (minSpend_.isNegative()) {
            throw std::invalid_argument("Coupon: minimum spend must not be negative");
        }
    }

    const std::string &code() const {
        return code_;
    }

    int basisPoints() const {
        return basisPoints_;
    }

    const Money &minSpend() const {
        return minSpend_;
    }

    bool matches(const std::string &candidate) const {
        return normalize(candidate) == normalize(code_);
    }

    // Throws when the cart does not meet the coupon's minimum spend. Discount
    // is floored to poisha; the cap of 100% keeps it from exceeding the base.
    Money discountFor(const Money &itemSubtotal) const {
        if (itemSubtotal.isNegative()) {
            throw std::invalid_argument("Coupon: item subtotal must not be negative");
        }
        if (itemSubtotal < minSpend_) {
            throw CheckoutError("coupon " + code_ + " needs a minimum spend of "
                                + minSpend_.format() + ", cart is " + itemSubtotal.format());
        }
        if (itemSubtotal.minor() > 0 && basisPoints_ > 0
            && itemSubtotal.minor() > std::numeric_limits<Money::Minor>::max() / basisPoints_) {
            throw std::overflow_error("Coupon: discount overflow");
        }
        return Money::fromMinor(itemSubtotal.minor() * basisPoints_ / MAX_BASIS_POINTS);
    }

    std::string describe() const;

private:
    static std::string normalize(const std::string &text) {
        std::string lowered;
        lowered.reserve(text.size());
        for (unsigned char byte : text) {
            lowered.push_back(static_cast<char>(std::tolower(byte)));
        }
        return lowered;
    }

    static constexpr int MAX_BASIS_POINTS = 10000;

    std::string code_;
    int basisPoints_;
    Money minSpend_;
};

inline std::string Coupon::describe() const {
    std::ostringstream out;
    out << code_ << " — " << (basisPoints_ / 100);
    if (basisPoints_ % 100 != 0) {
        out << '.' << std::setw(2) << std::setfill('0') << (basisPoints_ % 100);
    }
    out << "% off over " << minSpend_.format();
    return out.str();
}

#endif // QC_COUPON_HPP
