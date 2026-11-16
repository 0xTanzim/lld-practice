#ifndef QC_CHECKOUT_ERROR_HPP
#define QC_CHECKOUT_ERROR_HPP

#include <stdexcept>
#include <string>

// Every way a checkout can be refused. One type is enough: the customer acts on
// the message, not on the class. The original had no failure path at all - a
// declined payment still printed a success line.
class CheckoutError : public std::runtime_error {
public:
    explicit CheckoutError(const std::string &message) : std::runtime_error(message) {}
};

#endif // QC_CHECKOUT_ERROR_HPP
