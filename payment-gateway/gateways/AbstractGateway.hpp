#ifndef ABSTRACT_GATEWAY_HPP
#define ABSTRACT_GATEWAY_HPP

#include <memory>
#include "../banking/BankingSystem.hpp"
#include "../utils/ConsoleStyle.hpp"
#include "PaymentGateway.hpp"
using namespace std;

class AbstractGateway : public PaymentGateway {
protected:
    unique_ptr<BankingSystem> bankingSystem;

    virtual bool validatePayment(const PaymentRequest &request) = 0;
    virtual bool initiatePayment(const PaymentRequest &request) = 0;
    virtual bool confirmPayment(const PaymentRequest &request) = 0;

public:
    // Template method: subclasses supply the steps, never the order.
    PaymentResult processPayment(const PaymentRequest &request) override {
        if (!validatePayment(request)) {
            Console::fail(string(name()) + ": validation failed for " + request.sender);
            return {false, false, "validation failed"};
        }
        if (!initiatePayment(request)) {
            Console::fail(string(name()) + ": payment initiation failed for " + request.sender);
            return {false, true, "initiation failed"};
        }
        if (!confirmPayment(request)) {
            Console::fail(string(name()) + ": confirmation failed for " + request.sender);
            return {false, true, "confirmation failed"};
        }
        return {true, false, "ok"};
    }
};

#endif // ABSTRACT_GATEWAY_HPP
