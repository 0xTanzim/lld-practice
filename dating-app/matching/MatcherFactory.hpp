#ifndef DATING_MATCHER_FACTORY_HPP
#define DATING_MATCHER_FACTORY_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../matching/Matcher.hpp"
#include "../matching/ScoringRule.hpp"
#include "../matching/rules/PreferenceGate.hpp"
#include "../matching/rules/Proximity.hpp"
#include "../matching/rules/SharedInterests.hpp"

enum class MatcherType {
    BASIC,
    INTERESTS_BASED,
    LOCATION_BASED
};

// Builds a matcher out of rules. The rules themselves never know about each
// other; this function is the one place that decides which combinations exist.
//
// The original factory answered an unknown MatcherType with a BasicMatcher -
// a typo silently downgraded matching and nobody could tell.
class MatcherFactory {
public:
    static Matcher create(MatcherType type) {
        std::vector<std::unique_ptr<ScoringRule>> rules;
        rules.push_back(std::make_unique<PreferenceGate>());

        switch (type) {
        case MatcherType::BASIC:
            break;
        case MatcherType::INTERESTS_BASED:
            rules.push_back(std::make_unique<SharedInterests>());
            break;
        case MatcherType::LOCATION_BASED:
            rules.push_back(std::make_unique<SharedInterests>());
            rules.push_back(std::make_unique<Proximity>());
            break;
        default:
            throw std::invalid_argument("MatcherFactory: unknown matcher type "
                                        + std::to_string(static_cast<int>(type)));
        }

        return Matcher(std::move(rules));
    }

    static std::string name(MatcherType type) {
        switch (type) {
        case MatcherType::BASIC:
            return "basic";
        case MatcherType::INTERESTS_BASED:
            return "interests";
        case MatcherType::LOCATION_BASED:
            return "location";
        }
        throw std::invalid_argument("MatcherFactory: no name for that matcher type");
    }
};

#endif // DATING_MATCHER_FACTORY_HPP
