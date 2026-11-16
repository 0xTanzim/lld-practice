#ifndef QC_DARK_STORE_HPP
#define QC_DARK_STORE_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../models/Inventory.hpp"
#include "../models/Location.hpp"
#include "../managers/RiderPool.hpp"

// One fulfilment centre. It owns stock quantities and riders, and nothing
// else: the product definition lives in the Catalog, so a store cannot hold a
// stale copy of a name or price.
class DarkStore {
public:
    DarkStore(std::string name, Location location)
        : name_(std::move(name)), location_(location) {
        if (name_.empty()) {
            throw std::invalid_argument("DarkStore: name must not be empty");
        }
    }

    const std::string &name() const {
        return name_;
    }

    const Location &location() const {
        return location_;
    }

    double distanceTo(const Location &drop) const {
        return location_.distanceTo(drop);
    }

    Inventory &inventory() {
        return inventory_;
    }

    const Inventory &inventory() const {
        return inventory_;
    }

    RiderPool &riders() {
        return riders_;
    }

    const RiderPool &riders() const {
        return riders_;
    }

private:
    std::string name_;
    Location location_;
    Inventory inventory_;
    RiderPool riders_;
};

#endif // QC_DARK_STORE_HPP
