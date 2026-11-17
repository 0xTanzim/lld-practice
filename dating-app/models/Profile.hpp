#ifndef DATING_PROFILE_HPP
#define DATING_PROFILE_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Gender.hpp"
#include "Interest.hpp"
#include "Location.hpp"

// Everything a candidate sees. The identity fields are set once in the
// constructor, so a profile with an empty name, a missing age or no location
// cannot exist. The original allowed a half-built profile and then matched on it.
class Profile {
public:
    Profile(std::string name, int age, Gender gender, std::string bio, Location location)
        : name_(std::move(name)), age_(age), gender_(gender), bio_(std::move(bio)),
          location_(std::move(location)) {
        if (name_.empty()) {
            throw std::invalid_argument("Profile: name must not be empty");
        }
        if (age_ < MIN_AGE || age_ > MAX_AGE) {
            throw std::invalid_argument("Profile: age must be between " + std::to_string(MIN_AGE)
                                        + " and " + std::to_string(MAX_AGE));
        }
    }

    const std::string &name() const {
        return name_;
    }

    int age() const {
        return age_;
    }

    Gender gender() const {
        return gender_;
    }

    const std::string &bio() const {
        return bio_;
    }

    const Location &location() const {
        return location_;
    }

    const std::vector<std::string> &photos() const {
        return photos_;
    }

    const std::vector<Interest> &interests() const {
        return interests_;
    }

    void addPhoto(std::string url) {
        if (url.empty()) {
            throw std::invalid_argument("Profile: photo url must not be empty");
        }
        for (const std::string &existing : photos_) {
            if (existing == url) {
                throw std::invalid_argument("Profile: photo already added: " + url);
            }
        }
        photos_.push_back(std::move(url));
    }

    void addInterest(Interest interest) {
        if (hasInterest(interest.name())) {
            throw std::invalid_argument("Profile: interest already added: " + interest.name());
        }
        interests_.push_back(std::move(interest));
    }

    bool hasInterest(const std::string &name) const {
        for (const Interest &interest : interests_) {
            if (interest.name() == name) {
                return true;
            }
        }
        return false;
    }

private:
    static constexpr int MIN_AGE = 18;
    static constexpr int MAX_AGE = 120;

    std::string name_;
    int age_;
    Gender gender_;
    std::string bio_;
    Location location_;
    std::vector<std::string> photos_;
    std::vector<Interest> interests_;
};

#endif // DATING_PROFILE_HPP
