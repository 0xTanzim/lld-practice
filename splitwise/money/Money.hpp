#ifndef SPLITWISE_MONEY_HPP
#define SPLITWISE_MONEY_HPP

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

// Money is stored in paise (1/100 rupee), never as a double.
//
// The original kept every amount in `double`. Splitting Rs 800 four ways gave
// 199.99999999999997 per head, so the four shares did not add up to the 800 that
// was paid, and every comparison carried an `abs(x) < 0.01` fudge. Integers
// cannot drift: a share is a whole number of paise, and the sum of the shares is
// checked against the total by construction.
//
// Conversion happens at exactly two edges - user input and printed output - and
// nowhere else.
namespace Money {
using Amount = long long;

constexpr Amount PAISE_PER_RUPEE = 100;

// Half-up rounding of a rupee figure the caller typed.
inline Amount fromRupees(double rupees) {
    return static_cast<Amount>(std::llround(rupees * PAISE_PER_RUPEE));
}

inline double toRupees(Amount amount) {
    return static_cast<double>(amount) / PAISE_PER_RUPEE;
}

// "Rs 266.67", "-Rs 40.00". Built from integers so no float ever formats it.
inline std::string format(Amount amount) {
    const Amount absolute = amount < 0 ? -amount : amount;
    std::ostringstream out;
    out << (amount < 0 ? "-Rs " : "Rs ") << absolute / PAISE_PER_RUPEE << '.' << std::setw(2)
        << std::setfill('0') << absolute % PAISE_PER_RUPEE;
    return out.str();
}
}

#endif // SPLITWISE_MONEY_HPP
