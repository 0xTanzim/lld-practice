#ifndef PAYMENT_GATEWAY_HPP
#define PAYMENT_GATEWAY_HPP

#include "../models/PaymentRequest.hpp"
#include "../models/PaymentResult.hpp"

class PaymentGateway {
public:
    virtual ~PaymentGateway() = default;
    virtual const char *name() const = 0;
    virtual PaymentResult processPayment(const PaymentRequest &request) = 0;
};

#endif // PAYMENT_GATEWAY_HPP
