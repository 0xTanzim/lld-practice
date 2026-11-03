#ifndef SUBSCRIPTION_STATES_HPP
#define SUBSCRIPTION_STATES_HPP

#include <memory>
#include "ISubscriptionState.hpp"
using namespace std;

class ActiveState;
class PastDueState;
class CancelledState;

class ActiveState : public ISubscriptionState {
public:
    const char *name() const override {
        return "ACTIVE";
    }

    bool canBill() const override {
        return true;
    }

    unique_ptr<ISubscriptionState> onPaymentSuccess() override;
    unique_ptr<ISubscriptionState> onPaymentFailure(int failureStreak) override;
};

class PastDueState : public ISubscriptionState {
public:
    const char *name() const override {
        return "PAST_DUE";
    }

    bool canBill() const override {
        return true;
    }

    unique_ptr<ISubscriptionState> onPaymentSuccess() override;
    unique_ptr<ISubscriptionState> onPaymentFailure(int failureStreak) override;
};

class CancelledState : public ISubscriptionState {
public:
    const char *name() const override {
        return "CANCELLED";
    }

    bool canBill() const override {
        return false;
    }

    unique_ptr<ISubscriptionState> onPaymentSuccess() override;
    unique_ptr<ISubscriptionState> onPaymentFailure(int failureStreak) override;
};

inline unique_ptr<ISubscriptionState> ActiveState::onPaymentSuccess() {
    return make_unique<ActiveState>();
}

inline unique_ptr<ISubscriptionState> ActiveState::onPaymentFailure(int failureStreak) {
    constexpr int MAX_FAILURES_BEFORE_PAST_DUE = 2;
    if (failureStreak >= MAX_FAILURES_BEFORE_PAST_DUE) {
        return make_unique<PastDueState>();
    }
    return make_unique<ActiveState>();
}

inline unique_ptr<ISubscriptionState> PastDueState::onPaymentSuccess() {
    return make_unique<ActiveState>();
}

inline unique_ptr<ISubscriptionState> PastDueState::onPaymentFailure(int) {
    return make_unique<CancelledState>();
}

inline unique_ptr<ISubscriptionState> CancelledState::onPaymentSuccess() {
    return make_unique<CancelledState>();
}

inline unique_ptr<ISubscriptionState> CancelledState::onPaymentFailure(int) {
    return make_unique<CancelledState>();
}

#endif // SUBSCRIPTION_STATES_HPP
