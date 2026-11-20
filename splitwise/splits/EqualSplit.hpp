#ifndef SPLITWISE_EQUAL_SPLIT_HPP
#define SPLITWISE_EQUAL_SPLIT_HPP

#include <cstddef>
#include <vector>

#include "../models/Split.hpp"
#include "../money/Money.hpp"
#include "SplitStrategy.hpp"

// Equal shares. The total does not always divide by the head count, so the
// leftover paise are handed out one each to the first participants in the order
// given. That rule is stated rather than implied: the shares must add up to the
// total, and whoever is listed first absorbs a paisa first.
class EqualSplit : public SplitStrategy {
public:
    std::vector<Split> split(Money::Amount total,
                             const std::vector<UserId> &participants) const override {
        requireSplittable(total, participants);

        const auto headCount = static_cast<Money::Amount>(participants.size());
        const Money::Amount share = total / headCount;
        const Money::Amount leftover = total % headCount;

        std::vector<Split> splits;
        splits.reserve(participants.size());
        for (std::size_t index = 0; index < participants.size(); ++index) {
            const Money::Amount extra = static_cast<Money::Amount>(index) < leftover ? 1 : 0;
            splits.emplace_back(participants[index], share + extra);
        }
        return splits;
    }
};

#endif // SPLITWISE_EQUAL_SPLIT_HPP
