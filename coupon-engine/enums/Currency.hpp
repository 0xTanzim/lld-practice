#ifndef COUPON_CURRENCY_HPP
#define COUPON_CURRENCY_HPP

enum class Currency {
    BDT,
    USD
};

inline const char *toString(Currency currency) {
    return currency == Currency::BDT ? "BDT" : "USD";
}

#endif // COUPON_CURRENCY_HPP
