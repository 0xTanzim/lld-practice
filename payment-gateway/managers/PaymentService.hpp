#ifndef PAYMENT_SERVICE_HPP
#define PAYMENT_SERVICE_HPP

#include <memory>
#include <stdexcept>
#include "../gateways/PaymentGateway.hpp"
#include "../observers/PaymentEventBus.hpp"
#include "../utils/Money.hpp"
using namespace std;

class PaymentService {
public:
    static PaymentService &getInstance() {
        static PaymentService instance;
        return instance;
    }

    PaymentEventBus &events() {
        return eventBus;
    }

    void setGateway(unique_ptr<PaymentGateway> newGateway) {
        gateway = move(newGateway);
    }

    bool processPayment(const PaymentRequest &request) {
        if (!gateway) {
            throw logic_error("No payment gateway selected.");
        }

        PaymentResult result = gateway->processPayment(request);
        eventBus.publish({result.success,
                          request.sender,
                          request.receiver,
                          Money::format(request.currency, request.amount),
                          gateway->name()});
        return result.success;
    }

private:
    PaymentService() = default;
    PaymentService(const PaymentService &) = delete;
    PaymentService &operator=(const PaymentService &) = delete;

    unique_ptr<PaymentGateway> gateway;
    PaymentEventBus eventBus;
};

#endif // PAYMENT_SERVICE_HPP
