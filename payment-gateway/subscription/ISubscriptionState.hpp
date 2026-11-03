#ifndef I_SUBSCRIPTION_STATE_HPP
#define I_SUBSCRIPTION_STATE_HPP

#include <memory>
using namespace std;

class ISubscriptionState {
public:
    virtual ~ISubscriptionState() = default;

    virtual const char *name() const = 0;
    virtual bool canBill() const = 0;
    virtual unique_ptr<ISubscriptionState> onPaymentSuccess() = 0;
    virtual unique_ptr<ISubscriptionState> onPaymentFailure(int failureStreak) = 0;
};

#endif // I_SUBSCRIPTION_STATE_HPP
