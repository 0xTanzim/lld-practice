#ifndef SPLITWISE_SPLIT_STRATEGY_HPP
#define SPLITWISE_SPLIT_STRATEGY_HPP

#include <cstddef>
#include <map>
#include <vector>

#include "../errors/AppError.hpp"
#include "../models/Ids.hpp"
#include "../models/Split.hpp"
#include "../money/Money.hpp"

// How one expense is divided. Each concrete strategy owns its own configuration
// and answers one question: given a total and a list of people, what does each
// person owe?
//
// The original passed `const vector<double>& values = {}` to every strategy as a
// defaulted virtual argument, so one parameter meant rupees for ExactSplit and
// percentages for PercentageSplit, and a default argument on a virtual function
// is resolved from the static type. Any value could be handed to any strategy
// and produce a plausible, wrong split.
class SplitStrategy {
public:
    virtual ~SplitStrategy() = default;

    // The returned shares always sum to `total` exactly. Where the rounding
    // lands is the strategy's decision, documented on each implementation.
    virtual std::vector<Split> split(Money::Amount total,
                                     const std::vector<UserId> &participants) const = 0;
};

// Checks both strategies share: something to divide, and no person listed twice.
inline void requireSplittable(Money::Amount total, const std::vector<UserId> &participants) {
    if (total <= 0) {
        throw AppError("split: total must be positive, got " + Money::format(total));
    }
    if (participants.empty()) {
        throw AppError("split: nobody to split between");
    }

    std::map<UserId, bool> seen;
    for (const UserId &participant : participants) {
        if (!seen.emplace(participant, true).second) {
            throw AppError("split: " + participant + " appears twice in the same expense");
        }
    }
}

#endif // SPLITWISE_SPLIT_STRATEGY_HPP
