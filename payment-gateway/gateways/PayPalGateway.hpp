#ifndef PAYPAL_GATEWAY_HPP
#define PAYPAL_GATEWAY_HPP

#include "AbstractGateway.hpp"
#include "../utils/Money.hpp"
#include "../banking/PayPalBanking.hpp"
using namespace std;

class PayPalGateway : public AbstractGateway {
public:
    PayPalGateway() {
        bankingSystem = make_unique<PayPalBanking>();
    }

    const char *name() const override {
        return "PayPal Gateway";
    }

protected:
    bool validatePayment(const PaymentRequest &request) override {
        Console::info(string(name()) + ": validating " + request.sender);
        return request.amount > 0 && request.currency == Currency::USD;
    }

    bool initiatePayment(const PaymentRequest &request) override {
        Console::info(string(name()) + ": initiating "
                      + Money::format(request.currency, request.amount));
        return bankingSystem->processPayment(request);
    }

    bool confirmPayment(const PaymentRequest &request) override {
        Console::ok(string(name()) + ": confirmed for " + request.sender);
        return true;
    }
};

#endif // PAYPAL_GATEWAY_HPP
