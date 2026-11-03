#ifndef SUBSCRIPTION_HPP
#define SUBSCRIPTION_HPP

#include <memory>
#include <string>
#include "../enums/Currency.hpp"
#include "../subscription/ISubscriptionState.hpp"
#include "../subscription/SubscriptionStates.hpp"
using namespace std;

class Subscription {
private:
    string id;
    string customer;
    string merchant;
    double amount;
    Currency currency;
    int intervalDays;
    int failureStreak = 0;
    unique_ptr<ISubscriptionState> state = make_unique<ActiveState>();

public:
    Subscription(const string &id, const string &customer, const string &merchant,
                 double amount, Currency currency, int intervalDays)
        : id(id), customer(customer), merchant(merchant),
          amount(amount), currency(currency), intervalDays(intervalDays) {}

    void handlePaymentResult(bool success) {
        failureStreak = success ? 0 : failureStreak + 1;
        state = success ? state->onPaymentSuccess()
                        : state->onPaymentFailure(failureStreak);
    }

    bool canBill() const {
        return state->canBill();
    }

    const string &getId() const { return id; }
    const string &getCustomer() const { return customer; }
    const string &getMerchant() const { return merchant; }
    double getAmount() const { return amount; }
    Currency getCurrency() const { return currency; }
    int getIntervalDays() const { return intervalDays; }
    int getFailureStreak() const { return failureStreak; }
    string getStateName() const { return state->name(); }
};

#endif // SUBSCRIPTION_HPP
