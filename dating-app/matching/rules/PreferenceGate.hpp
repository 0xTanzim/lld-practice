#ifndef DATING_PREFERENCE_GATE_HPP
#define DATING_PREFERENCE_GATE_HPP

#include <string>

#include "../../models/Preference.hpp"
#include "../../models/Profile.hpp"
#include "../../models/User.hpp"
#include "../RuleScore.hpp"
#include "../ScoringRule.hpp"

// The hard gate: both people must accept the other's gender, age and distance.
// A rejection here ends the evaluation - no bonus can rescue a pair that one of
// them has filtered out.
class PreferenceGate : public ScoringRule {
public:
    std::string name() const override {
        return "preference gate";
    }

    RuleScore evaluate(const User &viewer, const User &candidate) const override {
        if (!accepts(viewer, candidate) || !accepts(candidate, viewer)) {
            return RuleScore::reject();
        }
        return RuleScore::accept(BASE_SCORE);
    }

private:
    // Passing the gate is itself worth half the score - two people who clear
    // each other's filters are already a maybe.
    static constexpr double BASE_SCORE = 0.5;
    static bool accepts(const User &viewer, const User &candidate) {
        const Preference &preference = viewer.preference();
        const Profile &other = candidate.profile();
        const double distanceKm = viewer.profile().location().distanceKm(other.location());
        return preference.wantsGender(other.gender()) && preference.acceptsAge(other.age())
               && preference.acceptsDistance(distanceKm);
    }
};

#endif // DATING_PREFERENCE_GATE_HPP
