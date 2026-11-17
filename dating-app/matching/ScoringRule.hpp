#ifndef DATING_SCORING_RULE_HPP
#define DATING_SCORING_RULE_HPP

#include <string>

#include "../models/User.hpp"
#include "RuleScore.hpp"

// One independent reason a pair of people does or does not fit.
//
// The original chained the rules by instantiating one matcher inside another:
//
//     LocationBasedMatcher::calculateMatchScore(...) {
//         InterestsBasedMatcher inner;          // <- new object every call
//         double base = inner.calculateMatchScore(...);
//         ...
//
// So "location" secretly meant "location + interests + basic", you could not use
// interests without basic, and every new rule meant editing the class below it.
// Here a rule only answers for itself and the Matcher composes them.
class ScoringRule {
public:
    virtual ~ScoringRule() = default;

    virtual std::string name() const = 0;

    // `viewer` is the person looking, `candidate` is the person being considered.
    // Rules are always evaluated in that direction; a mutual rule checks both
    // directions itself.
    virtual RuleScore evaluate(const User &viewer, const User &candidate) const = 0;
};

#endif // DATING_SCORING_RULE_HPP
