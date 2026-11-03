#ifndef STRIPE_GATEWAY_HPP
#define STRIPE_GATEWAY_HPP

#include "AbstractGateway.hpp"
#include "../utils/Money.hpp"
#include "../banking/StripeBanking.hpp"
using namespace std;

class StripeGateway : public AbstractGateway {
public:
    StripeGateway() {
        bankingSystem = make_unique<StripeBanking>();
    }

    const char *name() const override {
        return "Stripe Gateway";
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

#endif // STRIPE_GATEWAY_HPP
