#ifndef DATING_SHARED_INTERESTS_HPP
#define DATING_SHARED_INTERESTS_HPP

#include <algorithm>
#include <cstddef>
#include <string>

#include "../../models/Interest.hpp"
#include "../../models/User.hpp"
#include "../RuleScore.hpp"
#include "../ScoringRule.hpp"

// Bonus for overlap: up to 0.3, scaled by how much of the longer interest list
// the two people share. Two people who each list three interests and share two
// get 0.3 * 2/3 on top of the gate's 0.5.
class SharedInterests : public ScoringRule {
public:
    std::string name() const override {
        return "shared interests";
    }

    RuleScore evaluate(const User &viewer, const User &candidate) const override {
        const std::vector<Interest> &mine = viewer.profile().interests();
        const std::vector<Interest> &theirs = candidate.profile().interests();

        std::size_t shared = 0;
        for (const Interest &interest : mine) {
            for (const Interest &other : theirs) {
                if (interest.name() == other.name()) {
                    ++shared;
                    break;
                }
            }
        }

        const std::size_t larger = std::max(mine.size(), theirs.size());
        if (larger == 0) {
            return RuleScore::accept(0.0);
        }
        return RuleScore::accept(SHARED_WEIGHT * static_cast<double>(shared)
                                 / static_cast<double>(larger));
    }

private:
    static constexpr double SHARED_WEIGHT = 0.3;
};

#endif // DATING_SHARED_INTERESTS_HPP
