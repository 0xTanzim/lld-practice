#ifndef QC_CASH_ON_DELIVERY_HPP
#define QC_CASH_ON_DELIVERY_HPP

#include <stdexcept>
#include <string>

#include "../money/Money.hpp"
#include "../payment/PaymentMethod.hpp"

// Nothing is captured at checkout; the rider collects at the door. The order
// is still recorded, but with a pending payment status so the difference
// between "order accepted" and "order paid" stays visible.
class CashOnDelivery : public PaymentMethod {
public:
    std::string name() const override {
        return "Cash on delivery";
    }

    PaymentResult charge(const Money &amount) override {
        if (amount.isNegative()) {
            throw std::invalid_argument("CashOnDelivery: charge amount must not be negative");
        }
        return PaymentResult::pending("COD", "collect " + amount.format() + " at the door");
    }
};

#endif // QC_CASH_ON_DELIVERY_HPP
