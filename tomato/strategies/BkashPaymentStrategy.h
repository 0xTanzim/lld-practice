#ifndef BKASH_PAYMENT_STRATEGY_H
#define BKASH_PAYMENT_STRATEGY_H

#include "PaymentStrategy.h"
#include "../utils/Currency.h"
#include <iostream>
#include <string>
using namespace std;

class BkashPaymentStrategy : public PaymentStrategy {
private:
    string mobile;

public:
    BkashPaymentStrategy(const string& mob) : mobile(mob) {}

    void pay(double amount) override {
        cout << "Paid " << Currency::SYMBOL << amount << " using bKash (" << mobile << ")" << endl;
    }
};

#endif // BKASH_PAYMENT_STRATEGY_H
