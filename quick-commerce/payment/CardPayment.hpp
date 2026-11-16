#ifndef QC_CARD_PAYMENT_HPP
#define QC_CARD_PAYMENT_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../errors/CheckoutError.hpp"
#include "../money/Money.hpp"
#include "../payment/PaymentMethod.hpp"

// Debit/credit card with an available credit line. This is the rail the demo
// uses to show a decline after stock and riders were already lined up.
class CardPayment : public PaymentMethod {
public:
    CardPayment(std::string label, std::string last4, Money creditLimit)
        : label_(std::move(label)),
          last4_(std::move(last4)),
          creditLimit_(creditLimit),
          spent_(Money::zero()),
          sequence_(0) {
        if (label_.empty()) {
            throw std::invalid_argument("CardPayment: label must not be empty");
        }
        if (last4_.size() != 4) {
            throw std::invalid_argument("CardPayment: last4 must be exactly four digits");
        }
        if (creditLimit_.isNegative()) {
            throw std::invalid_argument("CardPayment: credit limit must not be negative");
        }
    }

    std::string name() const override {
        return label_ + " ****" + last4_;
    }

    PaymentResult charge(const Money &amount) override {
        if (amount.isNegative()) {
            throw std::invalid_argument("CardPayment: charge amount must not be negative");
        }
        const Money remaining = creditLimit_ - spent_;
        if (remaining < amount) {
            throw CheckoutError("card ****" + last4_ + ": " + remaining.format()
                                + " available, charge is " + amount.format());
        }
        spent_ = spent_ + amount;
        return PaymentResult::captured("CARD-" + std::to_string(++sequence_),
                                       "authorised on " + name());
    }

    Money availableCredit() const {
        return creditLimit_ - spent_;
    }

private:
    std::string label_;
    std::string last4_;
    Money creditLimit_;
    Money spent_;
    int sequence_;
};

#endif // QC_CARD_PAYMENT_HPP
