#ifndef QC_MONEY_HPP
#define QC_MONEY_HPP

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

// Money is integer minor units (poisha); binary floating point is not used for
// prices because repeated percentage discounts would drift.
class Money {
public:
    using Minor = std::int64_t;

    Money() = delete;

    static Money zero() {
        return Money(0);
    }

    static Money taka(Minor major) {
        return Money(mulChecked(major, MINOR_PER_TAKA));
    }

    static Money fromMinor(Minor minor) {
        return Money(minor);
    }

    Minor minor() const {
        return minor_;
    }

    bool isZero() const {
        return minor_ == 0;
    }

    bool isNegative() const {
        return minor_ < 0;
    }

    Money operator+(const Money &other) const {
        return Money(addChecked(minor_, other.minor_));
    }

    Money operator-(const Money &other) const {
        return Money(subChecked(minor_, other.minor_));
    }

    Money operator*(int factor) const {
        if (factor < 0) {
            throw std::invalid_argument("Money: multiplier must not be negative");
        }
        return Money(mulChecked(minor_, factor));
    }

    bool operator==(const Money &other) const {
        return minor_ == other.minor_;
    }

    bool operator!=(const Money &other) const {
        return !(*this == other);
    }

    bool operator<(const Money &other) const {
        return minor_ < other.minor_;
    }

    bool operator<=(const Money &other) const {
        return !(other < *this);
    }

    bool operator>(const Money &other) const {
        return other < *this;
    }

    bool operator>=(const Money &other) const {
        return !(*this < other);
    }

    Money min(const Money &other) const {
        return *this < other ? *this : other;
    }

    std::string format() const;

private:
    explicit Money(Minor minor) : minor_(minor) {}

    static constexpr Minor MINOR_PER_TAKA = 100;
    static constexpr Minor THOUSANDS = 1000;

    static Minor addChecked(Minor left, Minor right) {
        if (right > 0 && left > std::numeric_limits<Minor>::max() - right) {
            throw std::overflow_error("Money: addition overflow");
        }
        if (right < 0 && left < std::numeric_limits<Minor>::min() - right) {
            throw std::overflow_error("Money: addition underflow");
        }
        return left + right;
    }

    static Minor subChecked(Minor left, Minor right) {
        if (right < 0 && left > std::numeric_limits<Minor>::max() + right) {
            throw std::overflow_error("Money: subtraction overflow");
        }
        if (right > 0 && left < std::numeric_limits<Minor>::min() + right) {
            throw std::overflow_error("Money: subtraction underflow");
        }
        return left - right;
    }

    static Minor mulChecked(Minor value, Minor factor) {
        if (factor < 0) {
            throw std::invalid_argument("Money: multiplier must not be negative");
        }
        if (value == 0 || factor == 0) {
            return 0;
        }
        if (value > 0) {
            if (factor > std::numeric_limits<Minor>::max() / value) {
                throw std::overflow_error("Money: multiplication overflow");
            }
        } else if (value == -1) {
            return -factor;
        } else if (factor > std::numeric_limits<Minor>::min() / value) {
            throw std::overflow_error("Money: multiplication underflow");
        }
        return value * factor;
    }

    Minor minor_ = 0;
};

inline std::string Money::format() const {
    const bool negative = minor_ < 0;
    const Minor absolute = negative ? -minor_ : minor_;
    const Minor majorPart = absolute / MINOR_PER_TAKA;
    const Minor minorPart = absolute % MINOR_PER_TAKA;

    std::string digits = std::to_string(majorPart);
    std::string grouped;
    grouped.reserve(digits.size() + digits.size() / 3);
    int position = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (position > 0 && position % 3 == 0) {
            grouped.push_back(',');
        }
        grouped.push_back(*it);
        ++position;
    }
    std::reverse(grouped.begin(), grouped.end());

    std::ostringstream out;
    out << (negative ? "-" : "") << "৳" << grouped << '.'
        << std::setw(2) << std::setfill('0') << minorPart;
    return out.str();
}

#endif // QC_MONEY_HPP
