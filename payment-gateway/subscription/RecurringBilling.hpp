#ifndef RECURRING_BILLING_HPP
#define RECURRING_BILLING_HPP

#include "../gateways/PaymentGateway.hpp"
#include "../models/PaymentRequest.hpp"
#include "../models/Subscription.hpp"
#include "../utils/ConsoleStyle.hpp"
#include "../utils/Money.hpp"
using namespace std;

class RecurringBilling {
public:
    bool runCycle(Subscription &subscription, PaymentGateway &gateway) const {
        Console::section("Recurring cycle — " + subscription.getId()
                         + " (" + subscription.getStateName() + ")");
        Console::kv("Customer", subscription.getCustomer());
        Console::kv("Merchant", subscription.getMerchant());
        Console::kv("Amount", Money::format(subscription.getCurrency(), subscription.getAmount()));
        Console::kv("Interval", to_string(subscription.getIntervalDays()) + " days");

        if (!subscription.canBill()) {
            Console::warn("subscription is " + subscription.getStateName() + " — billing skipped");
            return false;
        }

        PaymentRequest request(subscription.getCustomer(), subscription.getMerchant(),
                               subscription.getAmount(), subscription.getCurrency());
        bool success = gateway.processPayment(request).success;
        subscription.handlePaymentResult(success);

        if (success) {
            Console::ok("cycle charged — state: " + subscription.getStateName());
        } else {
            Console::fail("cycle failed — state: " + subscription.getStateName()
                          + ", streak: " + to_string(subscription.getFailureStreak()));
        }
        return success;
    }
};

#endif // RECURRING_BILLING_HPP
