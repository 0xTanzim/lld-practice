#ifndef PAYMENT_CONTROLLER_HPP
#define PAYMENT_CONTROLLER_HPP

#include "../enums/GatewayType.hpp"
#include "../managers/GatewayFactory.hpp"
#include "../managers/PaymentService.hpp"
#include "../models/PaymentRequest.hpp"
using namespace std;

class PaymentController {
public:
    static PaymentController &getInstance() {
        static PaymentController instance;
        return instance;
    }

    bool handlePayment(GatewayType type, const PaymentRequest &request) {
        PaymentService::getInstance().setGateway(
            GatewayFactory::getInstance().createGateway(type));
        return PaymentService::getInstance().processPayment(request);
    }

private:
    PaymentController() = default;
    PaymentController(const PaymentController &) = delete;
    PaymentController &operator=(const PaymentController &) = delete;
};

#endif // PAYMENT_CONTROLLER_HPP
