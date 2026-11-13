#ifndef COUPON_CHECKOUT_RECEIPT_HPP
#define COUPON_CHECKOUT_RECEIPT_HPP

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../coupons/DiscountApplication.hpp"
#include "../enums/Currency.hpp"
#include "../money/Money.hpp"

struct CheckoutReceipt {
    std::string customerName;
    Currency currency;
    Money original;
    std::vector<DiscountApplication> discounts;
    Money finalTotal;

    CheckoutReceipt(std::string name, Currency currencyCode, Money originalTotal,
                    std::vector<DiscountApplication> applied, Money payable)
        : customerName(std::move(name)),
          currency(currencyCode),
          original(std::move(originalTotal)),
          discounts(std::move(applied)),
          finalTotal(std::move(payable)) {
        verifyLedger();
    }

    Money saved() const {
        return original - finalTotal;
    }

private:
    void verifyLedger() const {
        Money applied = Money::zero(currency);
        for (const DiscountApplication &discount : discounts) {
            if (discount.amount.currency() != currency) {
                throw std::logic_error("CheckoutReceipt: discount currency mismatch");
            }
            applied = applied + discount.amount;
        }
        if (applied != original - finalTotal) {
            throw std::logic_error("CheckoutReceipt: discount ledger does not add up");
        }
        if (finalTotal.isNegative()) {
            throw std::logic_error("CheckoutReceipt: payable total is negative");
        }
    }
};

#endif // COUPON_CHECKOUT_RECEIPT_HPP
