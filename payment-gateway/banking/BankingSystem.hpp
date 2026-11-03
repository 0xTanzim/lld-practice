#ifndef BANKING_SYSTEM_HPP
#define BANKING_SYSTEM_HPP

#include "../models/PaymentRequest.hpp"

class BankingSystem {
public:
    virtual ~BankingSystem() = default;
    virtual bool processPayment(const PaymentRequest &request) = 0;
    virtual const char *name() const = 0;
};

#endif // BANKING_SYSTEM_HPP
