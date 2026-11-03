#ifndef DUTCH_BANGLA_GATEWAY_HPP
#define DUTCH_BANGLA_GATEWAY_HPP

#include "AbstractGateway.hpp"
#include "../utils/Money.hpp"
#include "../banking/DutchBanglaBanking.hpp"
using namespace std;

class DutchBanglaGateway : public AbstractGateway {
public:
    DutchBanglaGateway() {
        bankingSystem = make_unique<DutchBanglaBanking>();
    }

    const char *name() const override {
        return "Dutch-Bangla Gateway";
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

#endif // DUTCH_BANGLA_GATEWAY_HPP
