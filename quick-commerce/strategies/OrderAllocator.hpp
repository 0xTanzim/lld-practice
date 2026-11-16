#ifndef QC_ORDER_ALLOCATOR_HPP
#define QC_ORDER_ALLOCATOR_HPP

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../errors/CheckoutError.hpp"
#include "../models/Cart.hpp"
#include "../models/Location.hpp"
#include "../strategies/Allocation.hpp"
#include "../stores/DarkStore.hpp"
#include "../stores/StoreNetwork.hpp"

// Splits one cart across the dark stores near the drop point.
//
//   Pass 1: if a single store covers the whole cart, use it - one shipment,
//           one rider, one delivery fee.
//   Pass 2: otherwise walk the stores nearest first and take
//           min(available, outstanding) from each.
//
// Pass 2 answers the real question: the customer orders 100, the nearest store
// has 70, so take 70 there and 30 from the next store in range.
//
// This is not a Chain of Responsibility. A chain hands a request to the first
// handler that takes it; a cart is split *across* stores, so the arithmetic has
// to accumulate a remainder, and the whole order can still be refused at the
// end. Planning only - it reads availability and never moves stock.
class OrderAllocator {
public:
    OrderAllocator(StoreNetwork &network, double maxKm) : network_(network), maxKm_(maxKm) {
        if (maxKm_ <= 0.0) {
            throw std::invalid_argument("OrderAllocator: service radius must be positive");
        }
    }

    Allocation allocate(const Cart &cart, const Location &drop) const {
        if (cart.isEmpty()) {
            throw std::invalid_argument("OrderAllocator: cart is empty");
        }

        const std::vector<DarkStore *> stores = network_.storesWithin(drop, maxKm_);
        if (stores.empty()) {
            throw CheckoutError("no dark store within " + radius() + " of the drop point");
        }

        for (DarkStore *store : stores) {
            if (!coversEverything(*store, cart)) {
                continue;
            }
            Allocation single;
            ShipmentPlan plan;
            plan.store = store;
            for (const CartLine &line : cart.lines()) {
                plan.lines.emplace_back(line.product, line.quantity);
            }
            single.addShipment(std::move(plan));
            return single;
        }

        std::vector<int> remaining;
        remaining.reserve(cart.lines().size());
        for (const CartLine &line : cart.lines()) {
            remaining.push_back(line.quantity);
        }

        Allocation split;
        for (DarkStore *store : stores) {
            ShipmentPlan plan;
            plan.store = store;
            for (std::size_t index = 0; index < cart.lines().size(); ++index) {
                if (remaining[index] <= 0) {
                    continue;
                }
                const CartLine &line = cart.lines()[index];
                const int take =
                    std::min(store->inventory().available(line.product.sku()), remaining[index]);
                if (take <= 0) {
                    continue;
                }
                plan.lines.emplace_back(line.product, take);
                remaining[index] -= take;
            }
            if (!plan.lines.empty()) {
                split.addShipment(std::move(plan));
            }
        }

        std::string shortfall;
        for (std::size_t index = 0; index < remaining.size(); ++index) {
            if (remaining[index] <= 0) {
                continue;
            }
            if (!shortfall.empty()) {
                shortfall += ", ";
            }
            shortfall += "SKU " + std::to_string(cart.lines()[index].product.sku()) + " x"
                         + std::to_string(remaining[index]);
        }
        if (!shortfall.empty()) {
            throw CheckoutError("stores within " + radius() + " cannot fulfil the order — missing "
                                + shortfall);
        }

        return split;
    }

    double maxKm() const {
        return maxKm_;
    }

private:
    bool coversEverything(const DarkStore &store, const Cart &cart) const {
        for (const CartLine &line : cart.lines()) {
            if (store.inventory().available(line.product.sku()) < line.quantity) {
                return false;
            }
        }
        return true;
    }

    std::string radius() const {
        std::ostringstream out;
        out << std::fixed << std::setprecision(1) << maxKm_ << " km";
        return out.str();
    }

    StoreNetwork &network_;
    double maxKm_;
};

#endif // QC_ORDER_ALLOCATOR_HPP
