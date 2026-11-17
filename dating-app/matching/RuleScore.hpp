#ifndef DATING_RULE_SCORE_HPP
#define DATING_RULE_SCORE_HPP

#include <stdexcept>

// What one scoring rule says about one pair.
//
// `accepted` is a hard gate: a pair that fails it scores 0 no matter what the
// other rules think. `points` is a bonus in [0, 1]. Keeping "may this pair
// proceed" separate from "how well do they fit" is what lets the rules stay
// independent - the original hard-wired them into an inheritance chain instead.
struct RuleScore {
    bool accepted;
    double points;

    static RuleScore reject() {
        return RuleScore{false, 0.0};
    }

    static RuleScore accept(double points = 0.0) {
        if (points < 0.0 || points > 1.0) {
            throw std::invalid_argument("RuleScore: points must be between 0 and 1");
        }
        return RuleScore{true, points};
    }
};

#endif // DATING_RULE_SCORE_HPP
