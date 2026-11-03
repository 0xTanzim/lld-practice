#ifndef DUTCH_BANGLA_BANKING_HPP
#define DUTCH_BANGLA_BANKING_HPP

#include <iostream>
#include "BankingSystem.hpp"
#include "../utils/Money.hpp"
using namespace std;

class DutchBanglaBanking : public BankingSystem {
private:
    int callCount = 0;

public:
    const char *name() const override {
        return "Dutch-Bangla";
    }

    bool processPayment(const PaymentRequest &request) override {
        ++callCount;
        cout << "    " << name() << " rail: moving "
             << Money::format(request.currency, request.amount) << "..." << endl;
        // Simulated flakiness: fails once, then recovers.
        return callCount > 1;
    }
};

#endif // DUTCH_BANGLA_BANKING_HPP
