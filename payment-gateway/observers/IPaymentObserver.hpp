#ifndef IPAYMENT_OBSERVER_HPP
#define IPAYMENT_OBSERVER_HPP

#include <string>
using namespace std;

struct PaymentEvent {
    bool success;
    string sender;
    string receiver;
    string amount;
    string gateway;
};

class IPaymentObserver {
public:
    virtual ~IPaymentObserver() = default;
    virtual void onPaymentEvent(const PaymentEvent &event) = 0;
};

#endif // IPAYMENT_OBSERVER_HPP
