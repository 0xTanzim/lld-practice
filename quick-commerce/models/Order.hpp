#ifndef QC_ORDER_HPP
#define QC_ORDER_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../money/Money.hpp"
#include "../payment/PaymentMethod.hpp"

// The money side of one order, kept in one place so no caller can assemble a
// total that disagrees with its parts.
class CheckoutSummary {
public:
    CheckoutSummary(Money itemSubtotal, Money discount, Money deliveryFee, std::string couponCode)
        : itemSubtotal_(itemSubtotal),
          discount_(discount),
          deliveryFee_(deliveryFee),
          couponCode_(std::move(couponCode)),
          total_(itemSubtotal - discount + deliveryFee) {
        verify();
    }

    const Money &itemSubtotal() const {
        return itemSubtotal_;
    }

    const Money &discount() const {
        return discount_;
    }

    const Money &deliveryFee() const {
        return deliveryFee_;
    }

    const Money &total() const {
        return total_;
    }

    const std::string &couponCode() const {
        return couponCode_;
    }

    bool hasCoupon() const {
        return !couponCode_.empty();
    }

private:
    void verify() const {
        if (itemSubtotal_.isNegative() || discount_.isNegative() || deliveryFee_.isNegative()) {
            throw std::logic_error("CheckoutSummary: amounts must not be negative");
        }
        if (discount_ > itemSubtotal_) {
            throw std::logic_error("CheckoutSummary: discount exceeds the item subtotal");
        }
        if (total_ != itemSubtotal_ - discount_ + deliveryFee_) {
            throw std::logic_error("CheckoutSummary: total does not match its parts");
        }
        if (total_.isNegative()) {
            throw std::logic_error("CheckoutSummary: total is negative");
        }
    }

    Money itemSubtotal_;
    Money discount_;
    Money deliveryFee_;
    std::string couponCode_;
    Money total_;
};

// A shipped product with the price frozen at checkout time. Orders never point
// back at the catalog or the cart.
struct OrderLine {
    std::string productName;
    int sku;
    int quantity;
    Money unitPrice;

    Money lineTotal() const {
        return unitPrice * quantity;
    }
};

// One store's share of the order, its own rider, and its own distance. This is
// the entity the original was missing: it had a flat item list and a separate
// partner list with no link between them.
struct OrderShipment {
    std::string storeName;
    std::string riderName;
    double distanceKm;
    std::vector<OrderLine> lines;

    Money itemSubtotal() const {
        Money total = Money::zero();
        for (const OrderLine &line : lines) {
            total = total + line.lineTotal();
        }
        return total;
    }

    int unitCount() const {
        int total = 0;
        for (const OrderLine &line : lines) {
            total += line.quantity;
        }
        return total;
    }
};

class Order {
public:
    Order(int id, std::string customerName, std::vector<OrderShipment> shipments,
          CheckoutSummary summary, std::string paymentMethodName, PaymentOutcome outcome,
          std::string paymentReference)
        : id_(id),
          customerName_(std::move(customerName)),
          shipments_(std::move(shipments)),
          summary_(std::move(summary)),
          paymentMethodName_(std::move(paymentMethodName)),
          paymentOutcome_(outcome),
          paymentReference_(std::move(paymentReference)) {
        if (id_ <= 0) {
            throw std::invalid_argument("Order: id must be positive");
        }
        if (customerName_.empty()) {
            throw std::invalid_argument("Order: customer name must not be empty");
        }
        if (shipments_.empty()) {
            throw std::invalid_argument("Order: at least one shipment is required");
        }
        if (unitCount() <= 0) {
            throw std::invalid_argument("Order: at least one unit is required");
        }
        if (paymentReference_.empty()) {
            throw std::invalid_argument("Order: payment reference must not be empty");
        }
    }

    int id() const {
        return id_;
    }

    const std::string &customerName() const {
        return customerName_;
    }

    const std::vector<OrderShipment> &shipments() const {
        return shipments_;
    }

    const CheckoutSummary &summary() const {
        return summary_;
    }

    const std::string &paymentMethodName() const {
        return paymentMethodName_;
    }

    PaymentOutcome paymentOutcome() const {
        return paymentOutcome_;
    }

    bool isPaid() const {
        return paymentOutcome_ == PaymentOutcome::Captured;
    }

    const std::string &paymentReference() const {
        return paymentReference_;
    }

    std::size_t shipmentCount() const {
        return shipments_.size();
    }

    int unitCount() const {
        int total = 0;
        for (const OrderShipment &shipment : shipments_) {
            total += shipment.unitCount();
        }
        return total;
    }

private:
    int id_;
    std::string customerName_;
    std::vector<OrderShipment> shipments_;
    CheckoutSummary summary_;
    std::string paymentMethodName_;
    PaymentOutcome paymentOutcome_;
    std::string paymentReference_;
};

#endif // QC_ORDER_HPP
