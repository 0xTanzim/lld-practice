#ifndef BKASH_BANKING_HPP
#define BKASH_BANKING_HPP

#include <iostream>
#include "BankingSystem.hpp"
#include "../utils/Money.hpp"
using namespace std;

class BkashBanking : public BankingSystem {
public:
    const char *name() const override {
        return "bKash";
    }

    bool processPayment(const PaymentRequest &request) override {
        cout << "    " << name() << " rail: moving "
             << Money::format(request.currency, request.amount) << "..." << endl;
        return true;
    }
};

#endif // BKASH_BANKING_HPP
