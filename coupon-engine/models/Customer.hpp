#ifndef COUPON_CUSTOMER_HPP
#define COUPON_CUSTOMER_HPP

#include <stdexcept>
#include <string>
#include <utility>

struct Customer {
    std::string name;
    bool loyaltyMember;
    std::string paymentBank;

    Customer(std::string customerName, bool isLoyaltyMember, std::string bank)
        : name(std::move(customerName)), loyaltyMember(isLoyaltyMember), paymentBank(std::move(bank)) {
        if (name.empty()) {
            throw std::invalid_argument("Customer: name must not be empty");
        }
    }
};

#endif // COUPON_CUSTOMER_HPP
