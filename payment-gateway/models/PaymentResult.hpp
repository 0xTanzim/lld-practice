#ifndef PAYMENT_RESULT_HPP
#define PAYMENT_RESULT_HPP

#include <string>
using namespace std;

struct PaymentResult {
    bool success;
    bool retryable;
    string reason;
};

#endif // PAYMENT_RESULT_HPP
