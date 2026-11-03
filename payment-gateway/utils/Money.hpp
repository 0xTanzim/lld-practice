#ifndef MONEY_HPP
#define MONEY_HPP

#include <iomanip>
#include <sstream>
#include <string>
#include "../enums/Currency.hpp"
using namespace std;

namespace Money {
inline string format(Currency currency, double amount) {
    ostringstream out;
    out << fixed << setprecision(2)
        << (currency == Currency::USD ? "$" : "৳") << amount;
    return out.str();
}

inline string symbol(Currency currency) {
    return currency == Currency::USD ? "$" : "৳";
}
}

#endif // MONEY_HPP
