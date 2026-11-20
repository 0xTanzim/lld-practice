#ifndef SPLITWISE_SPLIT_SPEC_HPP
#define SPLITWISE_SPLIT_SPEC_HPP

#include <string>
#include <utility>
#include <vector>

#include "SplitType.hpp"

// What the caller asked for: which rule, and the numbers that rule needs.
//
// The numbers mean different things per rule, so they are built through the named
// constructors below and never assembled by hand:
//
//   SplitSpec::equal()                     no values
//   SplitSpec::exact({200, 300, 200})      rupees, one per participant
//   SplitSpec::percentage({30, 70})        percent, one per participant
struct SplitSpec {
    SplitType type = SplitType::Equal;
    std::vector<double> values;

    static SplitSpec equal() {
        return SplitSpec{SplitType::Equal, {}};
    }

    static SplitSpec exact(std::vector<double> rupees) {
        return SplitSpec{SplitType::Exact, std::move(rupees)};
    }

    static SplitSpec percentage(std::vector<double> percentages) {
        return SplitSpec{SplitType::Percentage, std::move(percentages)};
    }

    std::string describe() const {
        return name(type);
    }
};

#endif // SPLITWISE_SPLIT_SPEC_HPP
