#ifndef PAYMENT_EVENT_BUS_HPP
#define PAYMENT_EVENT_BUS_HPP

#include <algorithm>
#include <vector>
#include "IPaymentObserver.hpp"
using namespace std;

class PaymentEventBus {
private:
    vector<IPaymentObserver *> observers;

public:
    void addObserver(IPaymentObserver *observer) {
        observers.push_back(observer);
    }

    void removeObserver(IPaymentObserver *observer) {
        observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
    }

    void publish(const PaymentEvent &event) const {
        for (auto *observer : observers) {
            observer->onPaymentEvent(event);
        }
    }
};

#endif // PAYMENT_EVENT_BUS_HPP
