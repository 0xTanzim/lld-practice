#ifndef DATING_PREFERENCE_HPP
#define DATING_PREFERENCE_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "Gender.hpp"

// What one user is looking for. No default constructor: "18 to 100, any
// distance, anyone" was the original's silent default, which made an
// unconfigured profile match everybody.
//
// An empty gender list means "open to everyone". A list is otherwise the exact
// set of genders the user wants to see.
class Preference {
public:
    Preference(std::vector<Gender> interestedIn, int minAge, int maxAge, double maxDistanceKm)
        : interestedIn_(std::move(interestedIn)), minAge_(minAge), maxAge_(maxAge),
          maxDistanceKm_(maxDistanceKm) {
        if (minAge_ < MIN_AGE) {
            throw std::invalid_argument("Preference: minimum age is " + std::to_string(MIN_AGE));
        }
        if (maxAge_ < minAge_) {
            throw std::invalid_argument("Preference: maximum age is below the minimum age");
        }
        if (maxAge_ > MAX_AGE) {
            throw std::invalid_argument("Preference: maximum age is " + std::to_string(MAX_AGE));
        }
        if (!(maxDistanceKm_ > 0.0) || maxDistanceKm_ > EARTH_CIRCUMFERENCE_KM) {
            throw std::invalid_argument("Preference: max distance must be between 0 and "
                                        + std::to_string(static_cast<int>(EARTH_CIRCUMFERENCE_KM))
                                        + " km");
        }
    }

    bool wantsGender(Gender gender) const {
        if (interestedIn_.empty()) {
            return true;
        }
        for (Gender wanted : interestedIn_) {
            if (wanted == gender) {
                return true;
            }
        }
        return false;
    }

    bool acceptsAge(int age) const {
        return age >= minAge_ && age <= maxAge_;
    }

    bool acceptsDistance(double kilometres) const {
        return kilometres <= maxDistanceKm_;
    }

    const std::vector<Gender> &interestedIn() const {
        return interestedIn_;
    }

    int minAge() const {
        return minAge_;
    }

    int maxAge() const {
        return maxAge_;
    }

    double maxDistanceKm() const {
        return maxDistanceKm_;
    }

private:
    // The app is for adults; a profile below 18 is not a preference setting.
    static constexpr int MIN_AGE = 18;
    static constexpr int MAX_AGE = 120;
    static constexpr double EARTH_CIRCUMFERENCE_KM = 40075.0;

    std::vector<Gender> interestedIn_;
    int minAge_;
    int maxAge_;
    double maxDistanceKm_;
};

#endif // DATING_PREFERENCE_HPP
