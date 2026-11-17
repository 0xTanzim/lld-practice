#ifndef DATING_MATCHER_HPP
#define DATING_MATCHER_HPP

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../models/User.hpp"
#include "RuleScore.hpp"
#include "ScoringRule.hpp"

// Runs the rules in order and combines them.
//
// Any rule that rejects ends the evaluation with 0.0. Otherwise the points are
// summed and capped at 1.0, so a score always means the same thing:
//   0.0        no match
//   0.0 < s    the pair may proceed, s says how strongly
//
// The rules are supplied to the constructor, so the policy is data, not
// inheritance. Adding "distance from my work address" is one new rule class plus
// one line in the factory - no existing rule or matcher is touched.
class Matcher {
public:
    explicit Matcher(std::vector<std::unique_ptr<ScoringRule>> rules) : rules_(std::move(rules)) {
        if (rules_.empty()) {
            throw std::invalid_argument("Matcher: at least one rule is required");
        }
    }

    double score(const User &viewer, const User &candidate) const {
        if (viewer.id() == candidate.id()) {
            return 0.0;
        }
        double total = 0.0;
        for (const std::unique_ptr<ScoringRule> &rule : rules_) {
            const RuleScore result = rule->evaluate(viewer, candidate);
            if (!result.accepted) {
                return 0.0;
            }
            total += result.points;
        }
        return std::min(1.0, total);
    }

    bool accepts(const User &viewer, const User &candidate) const {
        return score(viewer, candidate) > 0.0;
    }

    std::vector<std::string> ruleNames() const {
        std::vector<std::string> names;
        names.reserve(rules_.size());
        for (const std::unique_ptr<ScoringRule> &rule : rules_) {
            names.push_back(rule->name());
        }
        return names;
    }

    std::string pipeline() const {
        std::string joined;
        for (const std::string &name : ruleNames()) {
            if (!joined.empty()) {
                joined += " → ";
            }
            joined += name;
        }
        return joined;
    }

private:
    std::vector<std::unique_ptr<ScoringRule>> rules_;
};

#endif // DATING_MATCHER_HPP
