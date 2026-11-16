#ifndef QC_CUSTOMER_HPP
#define QC_CUSTOMER_HPP

#include <stdexcept>
#include <string>
#include <utility>

#include "../models/Location.hpp"

struct Customer {
    std::string name;
    Location location;

    Customer(std::string customerName, Location customerLocation)
        : name(std::move(customerName)), location(customerLocation) {
        if (name.empty()) {
            throw std::invalid_argument("Customer: name must not be empty");
        }
    }
};

#endif // QC_CUSTOMER_HPP
