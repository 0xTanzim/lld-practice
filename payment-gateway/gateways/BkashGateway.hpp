#ifndef BKASH_GATEWAY_HPP
#define BKASH_GATEWAY_HPP

#include "AbstractGateway.hpp"
#include "../utils/Money.hpp"
#include "../banking/BkashBanking.hpp"
using namespace std;

class BkashGateway : public AbstractGateway {
public:
    BkashGateway() {
        bankingSystem = make_unique<BkashBanking>();
    }

    const char *name() const override {
        return "bKash Gateway";
    }

protected:
    bool validatePayment(const PaymentRequest &request) override {
        Console::info(string(name()) + ": validating " + request.sender);
        return request.amount > 0 && request.currency == Currency::BDT;
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

#endif // BKASH_GATEWAY_HPP
