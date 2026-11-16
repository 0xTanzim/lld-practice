#ifndef QC_ALLOCATION_HPP
#define QC_ALLOCATION_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../models/Product.hpp"
#include "../money/Money.hpp"
#include "../stores/DarkStore.hpp"

// One product and quantity that a single store will ship.
struct AllocatedLine {
    Product product;
    int quantity;

    AllocatedLine(Product item, int count) : product(std::move(item)), quantity(count) {
        if (quantity <= 0) {
            throw std::invalid_argument("AllocatedLine: quantity must be positive");
        }
    }

    Money lineTotal() const {
        return product.price() * quantity;
    }
};

// Everything one store ships for the order. One shipment means one rider and
// one delivery fee, which is why the allocator keeps this list short.
struct ShipmentPlan {
    DarkStore *store = nullptr;
    std::vector<AllocatedLine> lines;

    const std::string &storeName() const {
        if (store == nullptr) {
            throw std::logic_error("ShipmentPlan: no store attached");
        }
        return store->name();
    }

    Money itemSubtotal() const {
        Money total = Money::zero();
        for (const AllocatedLine &line : lines) {
            total = total + line.lineTotal();
        }
        return total;
    }

    int unitCount() const {
        int total = 0;
        for (const AllocatedLine &line : lines) {
            total += line.quantity;
        }
        return total;
    }
};

// The result of splitting one cart across stores: a list of shipments, each
// tied to exactly one store. The original had a flat item list plus a separate
// partner list, with no link between them.
class Allocation {
public:
    void addShipment(ShipmentPlan plan) {
        if (plan.store == nullptr) {
            throw std::invalid_argument("Allocation: shipment has no store");
        }
        if (plan.lines.empty()) {
            throw std::invalid_argument("Allocation: shipment has no lines");
        }
        shipments_.push_back(std::move(plan));
    }

    std::size_t shipmentCount() const {
        return shipments_.size();
    }

    const std::vector<ShipmentPlan> &shipments() const {
        return shipments_;
    }

    Money itemSubtotal() const {
        Money total = Money::zero();
        for (const ShipmentPlan &shipment : shipments_) {
            total = total + shipment.itemSubtotal();
        }
        return total;
    }

    int unitCount() const {
        int total = 0;
        for (const ShipmentPlan &shipment : shipments_) {
            total += shipment.unitCount();
        }
        return total;
    }

private:
    std::vector<ShipmentPlan> shipments_;
};

#endif // QC_ALLOCATION_HPP
