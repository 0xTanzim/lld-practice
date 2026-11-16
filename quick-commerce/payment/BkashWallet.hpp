#ifndef QC_BKASH_WALLET_HPP
#define QC_BKASH_WALLET_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../errors/CheckoutError.hpp"
#include "../money/Money.hpp"
#include "../payment/PaymentMethod.hpp"

// Mobile wallet: the balance is the limit. A debit is only applied after the
// wallet has confirmed it can cover the whole charge.
class BkashWallet : public PaymentMethod {
public:
    BkashWallet(std::string account, Money balance)
        : account_(std::move(account)), balance_(balance), sequence_(0) {
        if (account_.empty()) {
            throw std::invalid_argument("BkashWallet: account must not be empty");
        }
        if (balance_.isNegative()) {
            throw std::invalid_argument("BkashWallet: balance must not be negative");
        }
    }

    std::string name() const override {
        return "bKash (" + account_ + ")";
    }

    PaymentResult charge(const Money &amount) override {
        if (amount.isNegative()) {
            throw std::invalid_argument("BkashWallet: charge amount must not be negative");
        }
        if (balance_ < amount) {
            throw CheckoutError("bKash: balance " + balance_.format() + " cannot cover "
                                + amount.format());
        }
        balance_ = balance_ - amount;
        return PaymentResult::captured("BKASH-" + std::to_string(++sequence_),
                                       "debited from wallet");
    }

    const Money &balance() const {
        return balance_;
    }

private:
    std::string account_;
    Money balance_;
    int sequence_;
};

#endif // QC_BKASH_WALLET_HPP
