#ifndef QC_RIDER_POOL_HPP
#define QC_RIDER_POOL_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "../models/Rider.hpp"

// Riders that belong to one store, plus how many orders each is carrying.
//
// Dispatch rule: the least-loaded rider with spare capacity takes the next
// shipment, earliest hire on a tie. That spreads work instead of filling one
// rider first, and it lets a rider batch two drops from the same store.
class RiderPool {
public:
    void hire(const Rider &rider) {
        if (indexOf(rider.id()) >= 0) {
            throw std::invalid_argument("RiderPool: duplicate rider id " + rider.id());
        }
        slots_.push_back(Slot{rider, 0});
    }

    // Gives the shipment to the least-loaded rider that still has room and
    // reports who took it. False when every rider is at capacity - the caller
    // decides whether that kills the order.
    bool assign(std::string &riderId, std::string &riderName) {
        const int count = static_cast<int>(slots_.size());
        int chosen = -1;
        for (int index = 0; index < count; ++index) {
            const Slot &slot = slots_[static_cast<std::size_t>(index)];
            if (slot.load >= slot.rider.capacity()) {
                continue;
            }
            if (chosen < 0 || slot.load < slots_[static_cast<std::size_t>(chosen)].load) {
                chosen = index;
            }
        }
        if (chosen < 0) {
            return false;
        }
        Slot &slot = slots_[static_cast<std::size_t>(chosen)];
        ++slot.load;
        riderId = slot.rider.id();
        riderName = slot.rider.name();
        return true;
    }

    void release(const std::string &riderId) {
        const int index = indexOf(riderId);
        if (index < 0) {
            throw std::out_of_range("RiderPool: unknown rider " + riderId);
        }
        Slot &slot = slots_[static_cast<std::size_t>(index)];
        if (slot.load <= 0) {
            throw std::logic_error("RiderPool: rider " + riderId + " carries no order");
        }
        --slot.load;
    }

    std::size_t idleCount() const {
        std::size_t count = 0;
        for (const Slot &slot : slots_) {
            if (slot.load == 0) {
                ++count;
            }
        }
        return count;
    }

    std::size_t availableCount() const {
        std::size_t count = 0;
        for (const Slot &slot : slots_) {
            if (slot.load < slot.rider.capacity()) {
                ++count;
            }
        }
        return count;
    }

    std::size_t size() const {
        return slots_.size();
    }

private:
    struct Slot {
        Rider rider;
        int load;
    };

    int indexOf(const std::string &riderId) const {
        const int count = static_cast<int>(slots_.size());
        for (int index = 0; index < count; ++index) {
            if (slots_[static_cast<std::size_t>(index)].rider.id() == riderId) {
                return index;
            }
        }
        return -1;
    }

    std::vector<Slot> slots_;
};

#endif // QC_RIDER_POOL_HPP
