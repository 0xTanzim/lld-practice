#ifndef STRIPE_BANKING_HPP
#define STRIPE_BANKING_HPP

#include <iostream>
#include "BankingSystem.hpp"
#include "../utils/Money.hpp"
using namespace std;

class StripeBanking : public BankingSystem {
private:
    int callCount = 0;

public:
    const char *name() const override {
        return "Stripe";
    }

    bool processPayment(const PaymentRequest &request) override {
        ++callCount;
        cout << "    " << name() << " rail: moving "
             << Money::format(request.currency, request.amount) << "..." << endl;
        // Simulated flakiness: fails twice, succeeds on 3rd attempt.
        return callCount > 2;
    }
};

#endif // STRIPE_BANKING_HPP
