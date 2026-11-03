#ifndef PAYMENT_LOGGER_HPP
#define PAYMENT_LOGGER_HPP

#include <string>
#include "IPaymentObserver.hpp"
#include "../utils/ConsoleStyle.hpp"
using namespace std;

class PaymentLogger : public IPaymentObserver {
public:
    void onPaymentEvent(const PaymentEvent &event) override {
        string line = event.gateway + " | " + event.sender + " → " + event.receiver
                      + " | " + event.amount;
        if (event.success) {
            Console::ok("[event] payment succeeded: " + line);
        } else {
            Console::fail("[event] payment failed: " + line);
        }
    }
};

#endif // PAYMENT_LOGGER_HPP
