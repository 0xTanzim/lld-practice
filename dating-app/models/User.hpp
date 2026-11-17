#ifndef DATING_USER_HPP
#define DATING_USER_HPP

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "../errors/AppError.hpp"
#include "Preference.hpp"
#include "Profile.hpp"
#include "Swipe.hpp"

// A person in the app: who they are, what they want, and what they have already
// swiped.
//
// Notification wiring is deliberately absent. In the original the constructor
// registered an observer with a global singleton and the destructor removed it,
// so the notification lifecycle lived inside User - improving User meant
// touching notifications, and destroying a User silently unsubscribed them.
// Channels are registered by the app facade instead.
class User {
public:
    User(std::string id, Profile profile, Preference preference)
        : id_(std::move(id)), profile_(std::move(profile)), preference_(std::move(preference)) {
        if (id_.empty()) {
            throw std::invalid_argument("User: id must not be empty");
        }
        if (id_ == profile_.name()) {
            // ids and display names are different things; this catches a caller
            // that passed the name twice.
            throw std::invalid_argument("User: id and display name must differ");
        }
    }

    const std::string &id() const {
        return id_;
    }

    const Profile &profile() const {
        return profile_;
    }

    const Preference &preference() const {
        return preference_;
    }

    // Records the swipe. Refuses to overwrite an earlier one: the original
    // stored `swipeHistory[otherId] = action`, so a second swipe flipped the
    // decision and every later match check saw the new value.
    void swipe(const std::string &targetId, SwipeAction action) {
        if (targetId == id_) {
            throw AppError("cannot swipe on yourself");
        }
        if (hasSwiped(targetId)) {
            throw AppError("already swiped on " + targetId);
        }
        swipes_.emplace(targetId, action);
    }

    bool hasSwiped(const std::string &targetId) const {
        return swipes_.find(targetId) != swipes_.end();
    }

    bool hasLiked(const std::string &targetId) const {
        const auto found = swipes_.find(targetId);
        return found != swipes_.end() && found->second == SwipeAction::RIGHT;
    }

    std::size_t swipeCount() const {
        return swipes_.size();
    }

private:
    std::string id_;
    Profile profile_;
    Preference preference_;
    std::map<std::string, SwipeAction> swipes_;
};

#endif // DATING_USER_HPP
