#ifndef PAYPAL_BANKING_HPP
#define PAYPAL_BANKING_HPP

#include <iostream>
#include "BankingSystem.hpp"
#include "../utils/Money.hpp"
using namespace std;

class PayPalBanking : public BankingSystem {
public:
    const char *name() const override {
        return "PayPal";
    }

    bool processPayment(const PaymentRequest &request) override {
        cout << "    " << name() << " rail: moving "
             << Money::format(request.currency, request.amount)
             << " (simulated outage)..." << endl;
        // Simulated outage: never succeeds — demonstrates retry exhaustion.
        return false;
    }
};

#endif // PAYPAL_BANKING_HPP
