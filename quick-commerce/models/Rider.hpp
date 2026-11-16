#ifndef QC_RIDER_HPP
#define QC_RIDER_HPP

#include <stdexcept>
#include <string>
#include <utility>

// A delivery rider. A rider can carry more than one order at a time, which is
// how quick-commerce batches drops that share a direction, so capacity is part
// of the rider rather than a global constant.
class Rider {
public:
    Rider(std::string id, std::string name, int capacity = 2)
        : id_(std::move(id)), name_(std::move(name)), capacity_(capacity) {
        if (id_.empty()) {
            throw std::invalid_argument("Rider: id must not be empty");
        }
        if (name_.empty()) {
            throw std::invalid_argument("Rider: name must not be empty");
        }
        if (capacity_ < 1) {
            throw std::invalid_argument("Rider: capacity must be at least 1");
        }
    }

    const std::string &id() const {
        return id_;
    }

    const std::string &name() const {
        return name_;
    }

    // Maximum number of orders this rider may carry at once.
    int capacity() const {
        return capacity_;
    }

private:
    std::string id_;
    std::string name_;
    int capacity_;
};

#endif // QC_RIDER_HPP
