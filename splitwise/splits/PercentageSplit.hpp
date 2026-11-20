#ifndef SPLITWISE_PERCENTAGE_SPLIT_HPP
#define SPLITWISE_PERCENTAGE_SPLIT_HPP

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../models/Split.hpp"
#include "../money/Money.hpp"
#include "SplitStrategy.hpp"

// Percentages, one per participant. They are stored as basis points (1/100 of a
// percent) so "adds up to 100%" is an integer comparison instead of a float
// comparison with a tolerance nobody can justify.
class PercentageSplit : public SplitStrategy {
public:
    explicit PercentageSplit(std::vector<double> percentages) {
        if (percentages.empty()) {
            throw AppError("percentage split: one percentage per participant is required");
        }

        long long totalBasisPoints = 0;
        for (const double percentage : percentages) {
            if (!std::isfinite(percentage) || percentage < 0.0 || percentage > 100.0) {
                throw AppError("percentage split: each share must be between 0 and 100");
            }
            const long long basisPoints = std::llround(percentage * 100.0);
            basisPoints_.push_back(basisPoints);
            totalBasisPoints += basisPoints;
        }

        if (totalBasisPoints != BASIS_POINTS_PER_PERCENT * 100) {
            throw AppError("percentage split: shares add up to " +
                           std::to_string(totalBasisPoints / BASIS_POINTS_PER_PERCENT) +
                           "%, expected 100%");
        }
    }

    // Each share is rounded to the nearest paisa and the last participant
    // absorbs the remainder, so the shares still sum to the total exactly.
    std::vector<Split> split(Money::Amount total,
                             const std::vector<UserId> &participants) const override {
        requireSplittable(total, participants);

        if (basisPoints_.size() != participants.size()) {
            throw AppError("percentage split: " + std::to_string(basisPoints_.size()) +
                           " percentage(s) for " + std::to_string(participants.size()) +
                           " participant(s)");
        }

        // Every share multiplies the total by up to 10,000 basis points. Above
        // this bound that product would overflow a long long and the shares would
        // silently come out wrong, so the expense is refused instead.
        constexpr Money::Amount MAX_SAFE_TOTAL =
            std::numeric_limits<Money::Amount>::max() / (BASIS_POINTS_PER_PERCENT * 100);
        if (total > MAX_SAFE_TOTAL) {
            throw AppError("percentage split: " + Money::format(total) + " is too large to divide");
        }

        std::vector<Split> splits;
        splits.reserve(participants.size());

        Money::Amount allocated = 0;
        for (std::size_t index = 0; index + 1 < participants.size(); ++index) {
            const Money::Amount share =
                (total * basisPoints_[index] + BASIS_POINTS_PER_PERCENT * 50) /
                (BASIS_POINTS_PER_PERCENT * 100);
            splits.emplace_back(participants[index], share);
            allocated += share;
        }

        // Checked before the share is constructed: a negative amount would be
        // rejected by Split with a message about the person, when the real
        // cause is the total being too small to divide.
        const Money::Amount remainder = total - allocated;
        if (remainder < 0) {
            throw AppError("percentage split: " + Money::format(total) +
                           " is too small to divide by these percentages");
        }
        splits.emplace_back(participants.back(), remainder);
        return splits;
    }

private:
    static constexpr long long BASIS_POINTS_PER_PERCENT = 100;

    std::vector<long long> basisPoints_;
};

#endif // SPLITWISE_PERCENTAGE_SPLIT_HPP
