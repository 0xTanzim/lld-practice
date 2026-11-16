#ifndef QC_STORE_NETWORK_HPP
#define QC_STORE_NETWORK_HPP

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../stores/DarkStore.hpp"

// Owns every dark store and answers the only geography question checkout
// asks: which stores can serve this drop point, nearest first?
class StoreNetwork {
public:
    // Stores are addressed by name afterwards. open() deliberately returns
    // nothing: the vector may reallocate on the next insert, so handing out a
    // reference here would be a dangling-pointer trap.
    void open(DarkStore store) {
        for (const DarkStore &existing : stores_) {
            if (existing.name() == store.name()) {
                throw std::invalid_argument("StoreNetwork: duplicate store " + store.name());
            }
        }
        stores_.push_back(std::move(store));
    }

    // Sorted by real distance, not by registration order, so the allocator
    // always prefers the store that can deliver fastest. Non-const because the
    // allocator that consumes this list then reserves stock through it.
    std::vector<DarkStore *> storesWithin(const Location &drop, double maxKm) {
        if (maxKm <= 0.0) {
            throw std::invalid_argument("StoreNetwork: service radius must be positive");
        }
        std::vector<std::pair<double, DarkStore *>> ranked;
        for (DarkStore &store : stores_) {
            const double distance = store.distanceTo(drop);
            if (distance <= maxKm) {
                ranked.emplace_back(distance, &store);
            }
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const auto &left, const auto &right) {
                      if (left.first != right.first) {
                          return left.first < right.first;
                      }
                      return left.second->name() < right.second->name();
                  });
        std::vector<DarkStore *> ordered;
        ordered.reserve(ranked.size());
        for (const auto &entry : ranked) {
            ordered.push_back(entry.second);
        }
        return ordered;
    }

    DarkStore *find(const std::string &name) {
        for (DarkStore &store : stores_) {
            if (store.name() == name) {
                return &store;
            }
        }
        return nullptr;
    }

    const std::vector<DarkStore> &stores() const {
        return stores_;
    }

    std::size_t size() const {
        return stores_.size();
    }

private:
    std::vector<DarkStore> stores_;
};

#endif // QC_STORE_NETWORK_HPP
