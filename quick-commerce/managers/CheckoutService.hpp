#ifndef QC_CHECKOUT_SERVICE_HPP
#define QC_CHECKOUT_SERVICE_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../errors/CheckoutError.hpp"
#include "../managers/OrderBook.hpp"
#include "../models/Cart.hpp"
#include "../models/Customer.hpp"
#include "../models/Order.hpp"
#include "../money/Money.hpp"
#include "../payment/PaymentMethod.hpp"
#include "../promotions/Coupon.hpp"
#include "../strategies/Allocation.hpp"
#include "../strategies/OrderAllocator.hpp"
#include "../stores/DarkStore.hpp"

// Facade over one checkout. The caller makes one call; this class owns the
// order of the steps:
//
//   allocate -> price -> reserve stock -> assign riders -> charge -> commit
//
// Template Method by composition: the sequence lives here, the two variable
// steps are handed in (allocation policy in the constructor, payment rail per
// call). Nothing before commit is permanent: if the payment is declined, a
// coupon rule fails, or a store has no rider, the catch block puts back every
// reservation made so far.
class CheckoutService {
public:
    CheckoutService(OrderAllocator allocator, OrderBook &orderBook, Money deliveryFeePerShipment)
        : allocator_(allocator), orderBook_(orderBook), deliveryFeePerShipment_(deliveryFeePerShipment) {
        if (deliveryFeePerShipment_.isNegative()) {
            throw std::invalid_argument("CheckoutService: delivery fee must not be negative");
        }
    }

    void addCoupon(const Coupon &coupon) {
        if (findCoupon(coupon.code()) != nullptr) {
            throw std::invalid_argument("CheckoutService: duplicate coupon code " + coupon.code());
        }
        coupons_.push_back(coupon);
    }

    std::size_t couponCount() const {
        return coupons_.size();
    }

    // Throws CheckoutError when the order cannot be placed, otherwise returns
    // the recorded order.
    Order checkout(const Customer &customer, const Cart &cart, PaymentMethod &payment,
                   const std::string &couponCode = "") {
        if (cart.isEmpty()) {
            throw CheckoutError("checkout: the cart is empty");
        }
        if (cart.subtotal().isZero()) {
            throw CheckoutError("checkout: the cart total is zero");
        }

        // Allocation and pricing are decisions, not side effects, so both run
        // before anything is reserved. A bad coupon or an unfillable cart costs
        // nothing here.
        const Allocation allocation = allocator_.allocate(cart, customer.location);
        const CheckoutSummary summary = price(allocation, couponCode);

        std::vector<ReservedStock> stock;
        std::vector<AssignedRider> riders;
        std::vector<std::string> riderNames;
        bool committed = false;

        try {
            for (const ShipmentPlan &plan : allocation.shipments()) {
                for (const AllocatedLine &line : plan.lines) {
                    plan.store->inventory().reserve(line.product.sku(), line.quantity);
                    stock.push_back(ReservedStock{plan.store, line.product.sku(), line.quantity});
                }
            }

            for (const ShipmentPlan &plan : allocation.shipments()) {
                std::string riderId;
                std::string riderName;
                if (!plan.store->riders().assign(riderId, riderName)) {
                    throw CheckoutError("no rider with spare capacity at " + plan.storeName()
                                        + " — the shipment cannot start");
                }
                riders.push_back(AssignedRider{plan.store, riderId});
                riderNames.push_back(riderName);
            }

            const PaymentResult result = charge(payment, summary.total());

            // Past this point the customer has been charged, so the order must
            // be recorded. Every commit was reserved by this checkout, so it
            // cannot fail here.
            for (const ReservedStock &entry : stock) {
                entry.store->inventory().commit(entry.sku, entry.quantity);
            }
            committed = true;

            Order order = buildOrder(customer, allocation, summary, payment, result, riderNames);
            orderBook_.add(std::move(order));
            return orderBook_.orders().back();
        } catch (...) {
            if (!committed) {
                for (std::size_t index = riders.size(); index > 0; --index) {
                    riders[index - 1].store->riders().release(riders[index - 1].riderId);
                }
                for (std::size_t index = stock.size(); index > 0; --index) {
                    stock[index - 1].store->inventory().release(stock[index - 1].sku,
                                                               stock[index - 1].quantity);
                }
            }
            throw;
        }
    }

private:
    struct ReservedStock {
        DarkStore *store;
        int sku;
        int quantity;
    };

    struct AssignedRider {
        DarkStore *store;
        std::string riderId;
    };

    const Coupon *findCoupon(const std::string &code) const {
        for (const Coupon &coupon : coupons_) {
            if (coupon.matches(code)) {
                return &coupon;
            }
        }
        return nullptr;
    }

    PaymentResult charge(PaymentMethod &payment, const Money &total) {
        if (total.isZero()) {
            return PaymentResult::captured("NO-CHARGE", "nothing to collect");
        }
        return payment.charge(total);
    }

    CheckoutSummary price(const Allocation &allocation, const std::string &couponCode) const {
        const Money subtotal = allocation.itemSubtotal();
        Money discount = Money::zero();
        std::string appliedCode;

        if (!couponCode.empty()) {
            const Coupon *coupon = findCoupon(couponCode);
            if (coupon == nullptr) {
                throw CheckoutError("unknown coupon code: " + couponCode);
            }
            const Money candidate = coupon->discountFor(subtotal);
            if (!candidate.isZero()) {
                discount = candidate;
                appliedCode = coupon->code();
            }
        }

        const Money fee = deliveryFeePerShipment_ * static_cast<int>(allocation.shipmentCount());
        return CheckoutSummary(subtotal, discount, fee, appliedCode);
    }

    Order buildOrder(const Customer &customer, const Allocation &allocation,
                     const CheckoutSummary &summary, const PaymentMethod &payment,
                     const PaymentResult &result,
                     const std::vector<std::string> &riderNames) const {
        if (riderNames.size() != allocation.shipmentCount()) {
            throw std::logic_error("CheckoutService: shipment and rider counts do not match");
        }

        std::vector<OrderShipment> shipments;
        Money checkedSubtotal = Money::zero();

        for (std::size_t index = 0; index < allocation.shipmentCount(); ++index) {
            const ShipmentPlan &plan = allocation.shipments()[index];
            OrderShipment shipment{plan.storeName(), riderNames[index],
                                   plan.store->distanceTo(customer.location), {}};
            for (const AllocatedLine &line : plan.lines) {
                shipment.lines.push_back(OrderLine{line.product.name(), line.product.sku(),
                                                  line.quantity, line.product.price()});
            }
            checkedSubtotal = checkedSubtotal + shipment.itemSubtotal();
            shipments.push_back(std::move(shipment));
        }

        if (checkedSubtotal != summary.itemSubtotal()) {
            throw std::logic_error("CheckoutService: order lines do not match the priced subtotal");
        }

        return Order(orderBook_.peekNextId(), customer.name, std::move(shipments), summary,
                     payment.name(), result.outcome, result.reference);
    }

    OrderAllocator allocator_;
    OrderBook &orderBook_;
    Money deliveryFeePerShipment_;
    std::vector<Coupon> coupons_;
};

#endif // QC_CHECKOUT_SERVICE_HPP
