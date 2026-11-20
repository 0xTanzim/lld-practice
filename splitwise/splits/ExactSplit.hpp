#ifndef SPLITWISE_EXACT_SPLIT_HPP
#define SPLITWISE_EXACT_SPLIT_HPP

#include <numeric>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../models/Split.hpp"
#include "../money/Money.hpp"
#include "SplitStrategy.hpp"

// Fixed amounts, one per participant. The caller states what each person owes,
// and the strategy checks that the statement adds up to the total the payer
// actually spent - the missing half of the original, which carried a
// `//validations` comment and no validation.
class ExactSplit : public SplitStrategy {
public:
    explicit ExactSplit(std::vector<Money::Amount> amounts) : amounts_(std::move(amounts)) {
        if (amounts_.empty()) {
            throw AppError("exact split: one amount per participant is required");
        }
        for (const Money::Amount amount : amounts_) {
            if (amount < 0) {
                throw AppError("exact split: amounts must not be negative");
            }
        }
    }

    std::vector<Split> split(Money::Amount total,
                             const std::vector<UserId> &participants) const override {
        requireSplittable(total, participants);

        if (amounts_.size() != participants.size()) {
            throw AppError("exact split: " + std::to_string(amounts_.size()) + " amount(s) for " +
                           std::to_string(participants.size()) + " participant(s)");
        }

        const Money::Amount stated = std::accumulate(amounts_.begin(), amounts_.end(), Money::Amount{0});
        if (stated != total) {
            throw AppError("exact split: amounts add up to " + Money::format(stated) +
                           " but the expense is " + Money::format(total));
        }

        std::vector<Split> splits;
        splits.reserve(participants.size());
        for (std::size_t index = 0; index < participants.size(); ++index) {
            splits.emplace_back(participants[index], amounts_[index]);
        }
        return splits;
    }

private:
    std::vector<Money::Amount> amounts_;
};

#endif // SPLITWISE_EXACT_SPLIT_HPP
