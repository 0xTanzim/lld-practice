#ifndef DATING_PROXIMITY_HPP
#define DATING_PROXIMITY_HPP

#include <algorithm>
#include <string>

#include "../../models/User.hpp"
#include "../RuleScore.hpp"
#include "../ScoringRule.hpp"

// Bonus for being close: up to 0.2, full marks at 0 km and nothing at the
// shorter of the two reach limits. "Shorter" because a match both people can act
// on is limited by whoever is willing to travel less.
class Proximity : public ScoringRule {
public:
    std::string name() const override {
        return "proximity";
    }

    RuleScore evaluate(const User &viewer, const User &candidate) const override {
        const double distanceKm =
            viewer.profile().location().distanceKm(candidate.profile().location());
        const double reachKm =
            std::min(viewer.preference().maxDistanceKm(), candidate.preference().maxDistanceKm());
        if (reachKm <= 0.0) {
            return RuleScore::accept(0.0);
        }
        const double closeness = std::clamp(1.0 - distanceKm / reachKm, 0.0, 1.0);
        return RuleScore::accept(PROXIMITY_WEIGHT * closeness);
    }

private:
    static constexpr double PROXIMITY_WEIGHT = 0.2;
};

#endif // DATING_PROXIMITY_HPP
