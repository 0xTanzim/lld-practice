#ifndef PAYMENT_REQUEST_HPP
#define PAYMENT_REQUEST_HPP

#include <string>
#include "../enums/Currency.hpp"
using namespace std;

struct PaymentRequest {
    string sender;
    string receiver;
    double amount;
    Currency currency;

    PaymentRequest(const string &sender, const string &receiver,
                   double amount, Currency currency)
        : sender(sender), receiver(receiver), amount(amount), currency(currency) {}
};

#endif // PAYMENT_REQUEST_HPP
