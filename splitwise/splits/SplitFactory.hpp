#ifndef SPLITWISE_SPLIT_FACTORY_HPP
#define SPLITWISE_SPLIT_FACTORY_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "../money/Money.hpp"
#include "EqualSplit.hpp"
#include "ExactSplit.hpp"
#include "PercentageSplit.hpp"
#include "SplitSpec.hpp"
#include "SplitStrategy.hpp"

// SplitSpec -> strategy. Returns an owner, not a raw pointer.
//
// The original returned `new EqualSplit()` and the caller never deleted it, and
// its `default:` branch returned an EqualSplit for any unrecognised type - a typo
// in a split type quietly became an equal split instead of an error.
class SplitFactory {
public:
    static std::unique_ptr<SplitStrategy> create(const SplitSpec &spec) {
        switch (spec.type) {
            case SplitType::Equal:
                if (!spec.values.empty()) {
                    throw AppError("split: equal split takes no values, got " +
                                   std::to_string(spec.values.size()));
                }
                return std::make_unique<EqualSplit>();

            case SplitType::Exact: {
                if (spec.values.empty()) {
                    throw AppError("split: exact split needs one rupee amount per participant");
                }
                std::vector<Money::Amount> amounts;
                amounts.reserve(spec.values.size());
                for (const double rupees : spec.values) {
                    amounts.push_back(Money::fromRupees(rupees));
                }
                return std::make_unique<ExactSplit>(std::move(amounts));
            }

            case SplitType::Percentage:
                if (spec.values.empty()) {
                    throw AppError("split: percentage split needs one share per participant");
                }
                return std::make_unique<PercentageSplit>(spec.values);
        }

        throw AppError("split: unknown split type");
    }
};

#endif // SPLITWISE_SPLIT_FACTORY_HPP
