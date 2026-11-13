#ifndef COUPON_MONEY_HPP
#define COUPON_MONEY_HPP

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../enums/Currency.hpp"

class Money {
public:
    using Minor = std::int64_t;

    Money() = delete;

    static Money zero(Currency currency) {
        return Money(0, currency);
    }

    static Money fromMajor(Minor major, Currency currency) {
        return Money(mulChecked(major, MINOR_PER_MAJOR), currency);
    }

    static Money fromMinor(Minor minor, Currency currency) {
        return Money(minor, currency);
    }

    Minor minor() const {
        return minor_;
    }

    Currency currency() const {
        return currency_;
    }

    bool isZero() const {
        return minor_ == 0;
    }

    bool isNegative() const {
        return minor_ < 0;
    }

    Money operator+(const Money &other) const {
        assertSameCurrency(other);
        return Money(addChecked(minor_, other.minor_), currency_);
    }

    Money operator-(const Money &other) const {
        assertSameCurrency(other);
        return Money(subChecked(minor_, other.minor_), currency_);
    }

    Money operator*(int factor) const {
        if (factor < 0) {
            throw std::invalid_argument("Money: quantity factor must not be negative");
        }
        return Money(mulChecked(minor_, factor), currency_);
    }

    bool operator==(const Money &other) const {
        assertSameCurrency(other);
        return minor_ == other.minor_;
    }

    bool operator!=(const Money &other) const {
        return !(*this == other);
    }

    bool operator<(const Money &other) const {
        assertSameCurrency(other);
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

    Money clampToZero() const {
        return isNegative() ? zero(currency_) : *this;
    }

    Money min(const Money &other) const {
        assertSameCurrency(other);
        return *this < other ? *this : other;
    }

    std::string format() const;

private:
    Money(Minor minor, Currency currency) : minor_(minor), currency_(currency) {}

    static constexpr Minor MINOR_PER_MAJOR = 100;
    static constexpr Minor THOUSAND = 1000;

    void assertSameCurrency(const Money &other) const {
        if (currency_ != other.currency_) {
            throw std::invalid_argument(std::string("Money: currency mismatch (")
                                        + toString(currency_) + " vs " + toString(other.currency_) + ")");
        }
    }

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
    Currency currency_ = Currency::BDT;
};

inline std::string Money::format() const {
    const char *symbol = currency_ == Currency::USD ? "$" : "৳";
    const bool negative = minor_ < 0;
    const Minor absolute = negative ? -minor_ : minor_;
    const Minor majorPart = absolute / MINOR_PER_MAJOR;
    const Minor minorPart = absolute % MINOR_PER_MAJOR;

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
    out << (negative ? "-" : "") << symbol << grouped << '.'
        << std::setw(2) << std::setfill('0') << minorPart;
    return out.str();
}

#endif // COUPON_MONEY_HPP
