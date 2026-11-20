#ifndef SPLITWISE_SPLIT_HPP
#define SPLITWISE_SPLIT_HPP

#include <string>
#include <utility>

#include "../errors/AppError.hpp"
#include "../money/Money.hpp"
#include "Ids.hpp"

// One person's share of one expense, in paise.
class Split {
public:
    Split(UserId participant, Money::Amount amount)
        : participant_(std::move(participant)), amount_(amount) {
        if (participant_.empty()) {
            throw AppError("split: participant must not be empty");
        }
        if (amount_ < 0) {
            throw AppError("split: share of " + participant_ + " must not be negative");
        }
    }

    const UserId &participant() const {
        return participant_;
    }

    Money::Amount amount() const {
        return amount_;
    }

private:
    UserId participant_;
    Money::Amount amount_;
};

#endif // SPLITWISE_SPLIT_HPP
