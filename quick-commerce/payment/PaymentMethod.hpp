#ifndef QC_PAYMENT_METHOD_HPP
#define QC_PAYMENT_METHOD_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../money/Money.hpp"

enum class PaymentOutcome {
    Captured, // money taken now
    Pending,  // cash on delivery: nothing captured, collected at the door
};

// What a payment rail reports back. `captured` decides whether the checkout
// may commit stock; `reference` is what a support agent would look for.
struct PaymentResult {
    PaymentOutcome outcome;
    std::string reference;
    std::string note;

    static PaymentResult captured(std::string reference, std::string note = "") {
        return PaymentResult{PaymentOutcome::Captured, std::move(reference), std::move(note)};
    }

    static PaymentResult pending(std::string reference, std::string note) {
        return PaymentResult{PaymentOutcome::Pending, std::move(reference), std::move(note)};
    }

    bool isCaptured() const {
        return outcome == PaymentOutcome::Captured;
    }
};

// Strategy: the checkout service charges a rail, it does not know which one.
class PaymentMethod {
public:
    virtual ~PaymentMethod() = default;

    virtual std::string name() const = 0;

    // Returns a result for a normal decline and throws PaymentFailed only for
    // a rail that cannot answer at all. Checkout treats both as a failure.
    virtual PaymentResult charge(const Money &amount) = 0;
};

#endif // QC_PAYMENT_METHOD_HPP
