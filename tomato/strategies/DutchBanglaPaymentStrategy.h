#ifndef DUTCH_BANGLA_PAYMENT_STRATEGY_H
#define DUTCH_BANGLA_PAYMENT_STRATEGY_H

#include "PaymentStrategy.h"
#include "../utils/Currency.h"
#include <iostream>
#include <string>
using namespace std;

class DutchBanglaPaymentStrategy : public PaymentStrategy {
private:
    string accountNumber;

public:
    DutchBanglaPaymentStrategy(const string& account) : accountNumber(account) {}

    void pay(double amount) override {
        cout << "Paid " << Currency::SYMBOL << amount << " using Dutch-Bangla Bank (" << accountNumber << ")" << endl;
    }
};

#endif // DUTCH_BANGLA_PAYMENT_STRATEGY_H
