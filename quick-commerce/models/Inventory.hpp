#ifndef QC_INVENTORY_HPP
#define QC_INVENTORY_HPP

#include <cstddef>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// Stock for one store. Quantities move through three states:
//
//   onHand   physical units in the store
//   reserved units promised to an order that has not been paid for yet
//   available = onHand - reserved
//
// A checkout reserves first, then commits or releases. That is what keeps a
// failed payment from destroying inventory, which the original did by
// removing stock before payment succeeded.
class Inventory {
public:
    struct Level {
        int onHand = 0;
        int reserved = 0;

        int available() const {
            return onHand - reserved;
        }
    };

    void stockIn(int sku, int quantity) {
        if (sku <= 0) {
            throw std::invalid_argument("Inventory: SKU must be positive");
        }
        if (quantity <= 0) {
            throw std::invalid_argument("Inventory: stock-in quantity must be positive");
        }
        Level &level = levels_[sku];
        if (level.onHand > std::numeric_limits<int>::max() - quantity) {
            throw std::overflow_error("Inventory: on-hand quantity overflow for SKU "
                                      + std::to_string(sku));
        }
        level.onHand += quantity;
    }

    int available(int sku) const {
        const Level *level = find(sku);
        return level == nullptr ? 0 : level->available();
    }

    int onHand(int sku) const {
        const Level *level = find(sku);
        return level == nullptr ? 0 : level->onHand;
    }

    int reserved(int sku) const {
        const Level *level = find(sku);
        return level == nullptr ? 0 : level->reserved;
    }

    bool hasStock(int sku) const {
        return available(sku) > 0;
    }

    void reserve(int sku, int quantity) {
        Level &level = requireLevel(sku, "reserve");
        if (quantity <= 0) {
            throw std::invalid_argument("Inventory: reservation quantity must be positive");
        }
        if (level.available() < quantity) {
            throw std::logic_error("Inventory: cannot reserve " + std::to_string(quantity)
                                   + " of SKU " + std::to_string(sku) + ", only "
                                   + std::to_string(level.available()) + " available");
        }
        level.reserved += quantity;
    }

    void release(int sku, int quantity) {
        Level &level = requireLevel(sku, "release");
        if (quantity <= 0) {
            throw std::invalid_argument("Inventory: release quantity must be positive");
        }
        if (level.reserved < quantity) {
            throw std::logic_error("Inventory: cannot release " + std::to_string(quantity)
                                   + " of SKU " + std::to_string(sku) + ", only "
                                   + std::to_string(level.reserved) + " reserved");
        }
        level.reserved -= quantity;
    }

    void commit(int sku, int quantity) {
        Level &level = requireLevel(sku, "commit");
        if (quantity <= 0) {
            throw std::invalid_argument("Inventory: commit quantity must be positive");
        }
        if (level.reserved < quantity) {
            throw std::logic_error("Inventory: cannot commit " + std::to_string(quantity)
                                   + " of SKU " + std::to_string(sku) + ", only "
                                   + std::to_string(level.reserved) + " reserved");
        }
        level.reserved -= quantity;
        level.onHand -= quantity;
    }

    std::vector<int> stockedSkus() const {
        std::vector<int> skus;
        for (const auto &entry : levels_) {
            if (entry.second.onHand > 0) {
                skus.push_back(entry.first);
            }
        }
        return skus;
    }

    std::size_t trackedSkuCount() const {
        return levels_.size();
    }

private:
    const Level *find(int sku) const {
        const auto it = levels_.find(sku);
        return it == levels_.end() ? nullptr : &it->second;
    }

    Level &requireLevel(int sku, const char *action) {
        if (sku <= 0) {
            throw std::invalid_argument("Inventory: SKU must be positive");
        }
        const auto it = levels_.find(sku);
        if (it == levels_.end()) {
            throw std::out_of_range(std::string("Inventory: cannot ") + action
                                    + " untracked SKU " + std::to_string(sku));
        }
        return it->second;
    }

    std::map<int, Level> levels_;
};

#endif // QC_INVENTORY_HPP
