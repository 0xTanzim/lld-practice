#ifndef COUPON_DISCOUNT_APPLICATION_HPP
#define COUPON_DISCOUNT_APPLICATION_HPP

#include <string>

#include "../money/Money.hpp"

struct DiscountApplication {
    std::string coupon;
    Money amount;
};

#endif // COUPON_DISCOUNT_APPLICATION_HPP
